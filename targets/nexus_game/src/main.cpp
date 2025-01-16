#include <SDL.h>
#include <SDL_ttf.h>
#include <nexus/client.h>
#include <nexus/nexus.h>
#include <nexus/server.h>

#include <iostream>
#include <stdexcept>
enum class MyMessageType : uint32_t
{
    EchoPlayer,
    GetPlayers,
    Move,
    MoveAccepted,
    MoveBroadcast,
    Reset,
    PlayerDisconnected,
    PlayerJoined,
};

void InitSDL()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        throw std::runtime_error("Error initializing SDL: " + std::string(SDL_GetError()));
    }

    if (TTF_Init() == -1)
    {
        throw std::runtime_error("Error initializing SDL_ttf: " + std::string(TTF_GetError()));
    }
}

void ShutdownSDL()
{
    TTF_Quit();
    SDL_Quit();
}

struct Player
{
    SDL_FPoint pos{0.0f, 0.0f};
    uint32_t ID;
    SDL_Color color{0, 255, 0, 255};
};
// Function to generate a random SDL_Color
SDL_Color GenerateRandomColor()
{
    // Seed the random number generator
    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    // Generate random RGB values (0 to 255)
    Uint8 r = static_cast<Uint8>(std::rand() % 256);
    Uint8 g = static_cast<Uint8>(std::rand() % 256);
    Uint8 b = static_cast<Uint8>(std::rand() % 256);

    // Return the color
    return SDL_Color{r, g, b, 255}; // Alpha is set to 255 (fully opaque)
}
void RunServer(uint16_t port)
{
    std::mutex playersMutex{};
    std::vector<Player> players{};
    SDL_Color bgColor = {140, 20, 7, 255}; // Red for no connections

    nxs::Server<MyMessageType> server{port};
    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
                           {
                               std::cout << "Client connected: ID = " << conn->GetID() << "\n";
                               {
                                   std::scoped_lock lock(playersMutex);
                                   players.push_back({{0.0f, 0.0f}, conn->GetID(), GenerateRandomColor()});
                                   nxs::Message<MyMessageType> echo{MyMessageType::EchoPlayer};
                                   echo << players.back();
                                   server.SendToClient(conn->GetID(), echo);
                               }

                               // Send all players to the newly connected client
                               nxs::Message<MyMessageType> newPlayerList{MyMessageType::GetPlayers};
                               {
                                   std::scoped_lock lock(playersMutex);
                                   for (const auto &p : players)
                                   {
                                       newPlayerList << p.pos.x << p.pos.y << p.ID << p.color.r << p.color.g << p.color.b << p.color.a;
                                   }
                               }
                               server.SendToClient(conn->GetID(), newPlayerList);

                                  // Update all other players about the new player
                                  nxs::Message<MyMessageType> newPlayerBroadcast{MyMessageType::PlayerJoined};
                                  {
                                      std::scoped_lock lock(playersMutex);
                                      const Player &newPlayer = players.back();
                                      newPlayerBroadcast << newPlayer;
                                  }
                                  server.Broadcast(newPlayerBroadcast, conn->GetID());

                               bgColor = {55, 82, 21, 255}; // Green for active connections
                           });

    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnDisconnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
                           {
    std::cout << "Client disconnected!\n";
    uint32_t disconnectedPlayerID = conn->GetID();

    // Remove the disconnected player from the server's list
    {
        std::scoped_lock l{playersMutex};
        std::erase_if(players, [&](Player p) { return p.ID == disconnectedPlayerID; });
    }

    // Broadcast the disconnection to all other players
    nxs::Message<MyMessageType> disconnectMsg{MyMessageType::PlayerDisconnected};
    disconnectMsg << disconnectedPlayerID;
    server.Broadcast(disconnectMsg);

    // Update background color if no players are left
    if (players.empty())
    {
        bgColor = {140, 20, 7, 255}; // Red for no connections
    } });

    server.AddMessageHandler(MyMessageType::Move, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn, nxs::Message<MyMessageType> &msg)
                             {
                                 SDL_FPoint newPos;
                                 msg >> newPos.y >> newPos.x;

                                 {
                                     std::scoped_lock lock(playersMutex);
                                     auto it = std::find_if(players.begin(), players.end(),
                                                            [&](const Player &p)
                                                            { return p.ID == conn->GetID(); });
                                     if (it != players.end())
                                     {
                                         it->pos = newPos;
                                     }
                                 }

                                 nxs::Message<MyMessageType> moveAccepted{MyMessageType::MoveAccepted};
                                 moveAccepted << newPos.x << newPos.y;
                                 conn->Send(moveAccepted);

                                 nxs::Message<MyMessageType> moveBroadcast{MyMessageType::MoveBroadcast};
                                 moveBroadcast << conn->GetID() << newPos.x << newPos.y;
                                 server.Broadcast(moveBroadcast, conn->GetID()); });
    server.AddMessageHandler(
        MyMessageType::Reset,
        [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn, nxs::Message<MyMessageType> &msg)
        {
            {
                std::scoped_lock l{playersMutex};
                auto player = std::find_if(players.begin(), players.end(),
                                           [&](Player p)
                                           { return p.ID == conn->GetID(); });
                player->pos = SDL_FPoint{0.0f, 0.0f};
            }
        });

    server.Run();

    SDL_Window *window = SDL_CreateWindow("Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font = TTF_OpenFont("resources/UnifontExMono.ttf", 12);
    if (!font)
    {
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
    }

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        for (const auto &p : players)
        {
            float side{25};
            SDL_FRect rect{p.pos.x - side / 2.0f, p.pos.y - side / 2.0f, side, side};

            // Render the player's square
            SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, p.color.a);
            SDL_RenderFillRectF(renderer, &rect);

            // Render the connection ID above the square
            SDL_Color textColor = {255, 255, 255, 255}; // White text color
            std::string connIdText = std::to_string(p.ID);
            SDL_Surface *textSurface = TTF_RenderText_Blended(font, connIdText.c_str(), textColor);
            if (textSurface)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture)
                {
                    SDL_Rect textRect;
                    textRect.x = static_cast<int>(p.pos.x - textSurface->w / 2.0f);
                    textRect.y = static_cast<int>(p.pos.y - side / 2.0f - textSurface->h);
                    textRect.w = textSurface->w;
                    textRect.h = textSurface->h;

                    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
        // Render the server's IP address
        SDL_Color adressTextColor = {255, 255, 255, 255}; // White text color
        std::string address = server.GetServerAddress();
        SDL_Surface *addressTextSurface =
            TTF_RenderUTF8_Blended(font, address.c_str(), adressTextColor);
        if (addressTextSurface)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, addressTextSurface);
            if (textTexture)
            {
                SDL_Rect textRect;
                textRect.x = 10;
                textRect.y = 10;
                textRect.w = addressTextSurface->w;
                textRect.h = addressTextSurface->h;

                SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(addressTextSurface);
        }
        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    server.Shutdown();
}

void RunClient(const std::string &serverAddress, uint16_t serverPort)
{
    Player player;
    std::vector<Player> otherPlayers;
    std::mutex playersMutex;
    nxs::Client<MyMessageType> client;

    client.Connect(serverAddress, serverPort);
    client.AddMessageHandler(MyMessageType::EchoPlayer, [&](nxs::Message<MyMessageType> &msg)
    {
        msg >> player;
    });
    client.AddMessageHandler(MyMessageType::PlayerJoined, [&](nxs::Message<MyMessageType> &msg)
    {
        Player newPlayer;
        msg >> newPlayer;
        otherPlayers.push_back(newPlayer);
    });
    client.AddMessageHandler(MyMessageType::GetPlayers, [&](nxs::Message<MyMessageType> &msg)
                             {
                                 std::scoped_lock lock(playersMutex);
                                 otherPlayers.clear();
                                 while (msg.m_Header.size != 0)
                                 {
                                     SDL_FPoint pos;
                                     uint32_t id;
                                     SDL_Color color;
                                     msg >> color.a >> color.b >> color.g >> color.r >> id >> pos.y >> pos.x;
                                     if (id != player.ID)
                                     {
                                         otherPlayers.push_back({pos, id, color});
                                     }
                                 } });

    client.AddMessageHandler(MyMessageType::MoveBroadcast, [&](nxs::Message<MyMessageType> &msg)
                             {
                                 uint32_t id;
                                 SDL_FPoint pos;
                                 msg >> pos.y >> pos.x >> id;

                                 std::scoped_lock lock(playersMutex);
                                 auto it = std::find_if(otherPlayers.begin(), otherPlayers.end(),
                                                        [&](const Player &p)
                                                        { return p.ID == id; });
                                 if (it != otherPlayers.end())
                                 {
                                     it->pos = pos;
                                 }
                                 else
                                 {
                                     //otherPlayers.push_back({pos, id, GenerateRandomColor()});
                                 } });
    client.AddMessageHandler(MyMessageType::MoveAccepted, [&](nxs::Message<MyMessageType> &message)
                             {
        SDL_FPoint acceptedPos;
        message >> acceptedPos.y >> acceptedPos.x;
        player.pos = acceptedPos; });

    client.AddMessageHandler(MyMessageType::PlayerDisconnected, [&](nxs::Message<MyMessageType> &message)
                             {
    uint32_t disconnectedPlayerID;
    message >> disconnectedPlayerID;

    // Remove the disconnected player from the list
    std::erase_if(otherPlayers, [&](Player p) { return p.ID == disconnectedPlayerID; });

    std::cout << "Player " << disconnectedPlayerID << " disconnected.\n"; });

    SDL_Window *window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("resources/UnifontExMono.ttf", 12);
    if (!font)
    {
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
    }
    bool running = true;
    while (running)
    {
        Uint32 frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Movement logic
        const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);
        SDL_FPoint newPos = player.pos;

        if (currentKeyStates[SDL_SCANCODE_UP])
        {
            newPos.y -= 1;
        }
        else if (currentKeyStates[SDL_SCANCODE_DOWN])
        {
            newPos.y += 1;
        }
        else if (currentKeyStates[SDL_SCANCODE_LEFT])
        {
            newPos.x -= 1;
        }
        else if (currentKeyStates[SDL_SCANCODE_RIGHT])
        {
            newPos.x += 1;
        }

        if (newPos.x != player.pos.x || newPos.y != player.pos.y)
        {
            nxs::Message<MyMessageType> msg{MyMessageType::Move};
            msg << newPos.x << newPos.y;
            client.Send(msg);
            player.pos = newPos; // Optimistic local update
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
        SDL_RenderClear(renderer);

        {
            std::scoped_lock lock(playersMutex);
            for (const auto &p : otherPlayers)
            {
                float side{25};
                SDL_FRect rect{p.pos.x - side / 2.0f, p.pos.y - side / 2.0f, side, side};

                // Render the player's square
                SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, p.color.a);
                SDL_RenderFillRectF(renderer, &rect);

                // Render the connection ID above the square
                SDL_Color textColor = {255, 255, 255, 255}; // White text color
                std::string connIdText = std::to_string(p.ID);
                SDL_Surface *textSurface = TTF_RenderText_Blended(font, connIdText.c_str(), textColor);
                if (textSurface)
                {
                    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture)
                    {
                        SDL_Rect textRect;
                        textRect.x = static_cast<int>(p.pos.x - textSurface->w / 2.0f);
                        textRect.y = static_cast<int>(p.pos.y - side / 2.0f - textSurface->h);
                        textRect.w = textSurface->w;
                        textRect.h = textSurface->h;

                        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                        SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                }
            }
            float side{25};
            SDL_FRect rect{player.pos.x - side / 2.0f, player.pos.y - side / 2.0f, side, side};
            SDL_SetRenderDrawColor(renderer, player.color.r, player.color.g, player.color.b, player.color.a);
            SDL_RenderFillRectF(renderer, &rect);
            // Render the connection ID above the square
            SDL_Color textColor = {255, 255, 255, 255}; // White text color
            std::string connIdText = std::to_string(player.ID);
            SDL_Surface *textSurface = TTF_RenderText_Blended(font, connIdText.c_str(), textColor);
            if (textSurface)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture)
                {
                    SDL_Rect textRect;
                    textRect.x = static_cast<int>(player.pos.x - textSurface->w / 2.0f);
                    textRect.y = static_cast<int>(player.pos.y - side / 2.0f - textSurface->h);
                    textRect.w = textSurface->w;
                    textRect.h = textSurface->h;

                    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }

        SDL_RenderPresent(renderer);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 1000 / 60)
        {
            SDL_Delay((1000 / 60) - frameTime); // Limit frame rate to 60 FPS
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    client.Disconnect();
    SDL_Delay(100); // Ensure clean disconnection
}
uint16_t ParsePort(const std::string &portStr)
{
    try
    {
        int port = std::stoi(portStr);
        if (port < 1 || port > 65535)
            throw std::out_of_range("Port must be in range 1-65535.");
        return static_cast<uint16_t>(port);
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument("Invalid port number: " + portStr);
    }
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0]
                  << " --server <optional: port> | --client <IP-address> <port>\n";
        return 1;
    }

    try
    {
        nxs::Nexus::Init();
        InitSDL();

        std::string mode = argv[1];
        if (mode == "--server")
        {
            uint16_t port = 60000;
            if (argc >= 3)
                port = ParsePort(argv[2]);
            RunServer(port);
        }
        else if (mode == "--client")
        {
            if (argc < 4)
            {
                std::cerr << "Usage for client: " << argv[0] << " --client <IP-address> <port>\n";
                return 1;
            }
            std::string ip = argv[2];
            uint16_t port = ParsePort(argv[3]);
            RunClient(ip, port);
        }
        else
        {
            std::cerr << "Invalid mode. Use --server or --client\n";
            return 1;
        }

        ShutdownSDL();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        ShutdownSDL();
        return 1;
    }

    return 0;
}