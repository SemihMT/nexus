#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <nexus/nexus.h>
#include <nexus/server.h>
#include <stdexcept>
#include <nexus/client.h>
enum class MyMessageType : uint32_t
{
    Move,
    MoveAccepted,
    Reset,
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
    SDL_Color color{0,255,0,255};

};
// Function to generate a random SDL_Color
SDL_Color GenerateRandomColor() {
    // Seed the random number generator
    static bool seeded = false;
    if (!seeded) {
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
void RunServer()
{
    std::vector<Player> players{};
    SDL_Color bgColor = {255, 0, 0, 255}; // Red for no connections

    nxs::Server<MyMessageType> server{60000};
    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
                           {
                               std::cout << "Client connected!\n";
                               players.emplace_back(Player{{0.0f, 0.0f}, conn->GetID(), GenerateRandomColor()});
                               bgColor = {0, 255, 0, 255}; // Green for active connections
                           });
    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnDisconnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
                           {
        std::cout << "Client disconnected!\n";
        std::erase_if(players, [&](Player p){return p.ID == conn->GetID();});
        if (players.size() == 0) {
            bgColor = {255, 0, 0, 255}; // Red for no connections
        } });
    server.AddMessageHandler(MyMessageType::Move, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn, nxs::Message<MyMessageType> &msg)
                             {
        SDL_FPoint newPos;
        msg >> newPos.y >> newPos.x;
        auto player = std::find_if(players.begin(), players.end(), [&](Player p){return p.ID == conn->GetID();});
        player->pos = newPos;
        nxs::Message<MyMessageType> moveAccept{MyMessageType::MoveAccepted};
        moveAccept << newPos.x << newPos.y;
        conn->Send(moveAccept); });
    server.AddMessageHandler(MyMessageType::Reset, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn, nxs::Message<MyMessageType> &msg)
                             {
        auto player = std::find_if(players.begin(), players.end(), [&](Player p){return p.ID == conn->GetID();});
        player->pos = SDL_FPoint{0.0f,0.0f}; });

    server.Run();

    SDL_Window *window = SDL_CreateWindow("Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
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
            SDL_FRect rect{p.pos.x - side/2.0f, p.pos.y + side/2.0f, side, side};

            SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, p.color.a);
            SDL_RenderDrawRectF(renderer,&rect);
        }
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    server.Shutdown();
}

void RunClient(const std::string &serverAddress, uint16_t serverPort) {
    Player p{};
    nxs::Client<MyMessageType> client;

    client.Connect(serverAddress, serverPort);
    client.AddMessageHandler(MyMessageType::MoveAccepted, [&](nxs::Message<MyMessageType>& message) {
        SDL_FPoint acceptedPos;
        message >> acceptedPos.y >> acceptedPos.x;
        p.pos = acceptedPos;
    });

    SDL_Window *window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }


        // Movement logic
        SDL_FPoint newPos = p.pos;
        const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);

        if (currentKeyStates[SDL_SCANCODE_UP]) {
            newPos.y -= 1;
        } else if (currentKeyStates[SDL_SCANCODE_DOWN]) {
            newPos.y += 1;
        } else if (currentKeyStates[SDL_SCANCODE_LEFT]) {
            newPos.x -= 1;
        } else if (currentKeyStates[SDL_SCANCODE_RIGHT]) {
            newPos.x += 1;
        }

        if (newPos.x != p.pos.x || newPos.y != p.pos.y) {
            nxs::Message<MyMessageType> msg{MyMessageType::Move};
            msg << newPos.x << newPos.y;
            client.Send(msg);
            p.pos = newPos; // Optimistic local update
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
        SDL_RenderClear(renderer);

        float side = 25;
        SDL_FRect rect{p.pos.x - side / 2.0f, p.pos.y - side / 2.0f, side, side};
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, p.color.a);
        SDL_RenderFillRectF(renderer, &rect);

        SDL_RenderPresent(renderer);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 1000 / 60) {
            SDL_Delay((1000 / 60) - frameTime); // Limit frame rate to 60 FPS
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    client.Disconnect();
    SDL_Delay(100); // Ensure clean disconnection
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    try
    {
        nxs::Nexus::Init();
        InitSDL();

        std::string mode = argv[1];
        if (mode == "--server")
        {
            RunServer();
        }
        else if (mode == "--client")
        {
            RunClient("127.0.0.1", 60000);
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