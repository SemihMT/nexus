#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include <nexus/client.h>
#include <nexus/nexus.h>
#include <nexus/server.h>

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <filesystem>

// Returns the absolute path to a file in the resources/ directory next to the
// executable, regardless of the process working directory.
// SDL_GetBasePath() returns the exe directory on all platforms (Windows, Linux,
// macOS) and is the correct cross-platform replacement for relative paths.
static std::string GetResourcePath(const std::string &filename)
{
    const char *base = SDL_GetBasePath();
    if (!base)
        throw std::runtime_error("SDL_GetBasePath failed: " + std::string(SDL_GetError()));
    std::filesystem::path p = std::filesystem::path(base) / "resources" / filename;
    return p.string();
}

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
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error("Error initializing SDL: " + std::string(SDL_GetError()));

    if (!TTF_Init())
        throw std::runtime_error("Error initializing SDL_ttf: " + std::string(SDL_GetError()));
}

void ShutdownSDL()
{
    TTF_Quit();
    SDL_Quit();
}

struct Player
{
    SDL_FPoint pos{0.0f, 0.0f};
    uint32_t ID{0};
    SDL_Color color{0, 255, 0, 255};
};

SDL_Color GenerateRandomColor()
{
    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    return SDL_Color{
        static_cast<Uint8>(std::rand() % 256),
        static_cast<Uint8>(std::rand() % 256),
        static_cast<Uint8>(std::rand() % 256),
        255};
}

// Helper: render a player square + ID label without acquiring any locks.
// Call only while holding stateMutex (or for single-threaded use).
static void RenderPlayer(SDL_Renderer *renderer, TTF_Font *font, const Player &p)
{
    constexpr float side = 25.0f;
    SDL_FRect rect{p.pos.x - side / 2.0f, p.pos.y - side / 2.0f, side, side};
    SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, p.color.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color white{255, 255, 255, 255};
    std::string label = std::to_string(p.ID);
    SDL_Surface *surf = TTF_RenderText_Blended(font, label.c_str(), label.length(), white);
    if (!surf)
        return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex)
    {
        SDL_FRect tr{
            p.pos.x - surf->w / 2.0f,
            p.pos.y - side / 2.0f - surf->h,
            surf->w, surf->h};
        SDL_RenderTexture(renderer, tex, nullptr, &tr);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(surf);
}

// ============================================================================
void RunServer(uint16_t port)
{
    // stateMutex guards: players, bgColor
    // All ASIO callbacks (server strand thread) and the SDL render loop
    // (main thread) must hold this lock before touching either.
    std::mutex stateMutex;
    std::vector<Player> players;
    SDL_Color bgColor{140, 20, 7, 255}; // red — no connections

    nxs::Server<MyMessageType> server{port};

    server.AddEventHandler(
        nxs::Server<MyMessageType>::ServerEvent::OnConnect,
        [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
        {
            std::scoped_lock lock(stateMutex);

            players.push_back({{0.0f, 0.0f}, conn->GetID(), GenerateRandomColor()});

            // Echo the new player's own data back to them
            nxs::Message<MyMessageType> echo{MyMessageType::EchoPlayer};
            echo << players.back();
            server.SendToClient(conn->GetID(), echo);

            // Send the full current player list to the newcomer
            nxs::Message<MyMessageType> list{MyMessageType::GetPlayers};
            for (const auto &p : players)
                list << p.pos.x << p.pos.y << p.ID << p.color.r << p.color.g << p.color.b << p.color.a;
            server.SendToClient(conn->GetID(), list);

            // Announce the newcomer to everyone else
            nxs::Message<MyMessageType> joined{MyMessageType::PlayerJoined};
            joined << players.back();
            server.Broadcast(joined, conn->GetID());

            bgColor = {55, 82, 21, 255}; // green — active connections
        });

    server.AddEventHandler(
        nxs::Server<MyMessageType>::ServerEvent::OnDisconnect,
        [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn)
        {
            uint32_t id = conn->GetID();

            nxs::Message<MyMessageType> msg{MyMessageType::PlayerDisconnected};
            msg << id;
            server.Broadcast(msg);

            std::scoped_lock lock(stateMutex);
            std::erase_if(players, [id](const Player &p)
                          { return p.ID == id; });
            if (players.empty())
                bgColor = {140, 20, 7, 255};
        });

    server.AddMessageHandler(
        MyMessageType::Move,
        [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn,
            nxs::Message<MyMessageType> &msg)
        {
            SDL_FPoint newPos;
            msg >> newPos.y >> newPos.x;

            {
                std::scoped_lock lock(stateMutex);
                auto it = std::find_if(players.begin(), players.end(),
                                       [&](const Player &p)
                                       { return p.ID == conn->GetID(); });
                if (it != players.end())
                    it->pos = newPos;
            }

            nxs::Message<MyMessageType> accepted{MyMessageType::MoveAccepted};
            accepted << newPos.x << newPos.y;
            conn->Send(accepted);

            nxs::Message<MyMessageType> broadcast{MyMessageType::MoveBroadcast};
            broadcast << conn->GetID() << newPos.x << newPos.y;
            server.Broadcast(broadcast, conn->GetID());
        });

    server.AddMessageHandler(
        MyMessageType::Reset,
        [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn,
            nxs::Message<MyMessageType> &)
        {
            std::scoped_lock lock(stateMutex);
            auto it = std::find_if(players.begin(), players.end(),
                                   [&](const Player &p)
                                   { return p.ID == conn->GetID(); });
            if (it != players.end())
                it->pos = {0.0f, 0.0f};
        });

    server.Run(); // non-blocking — starts accept loop, returns immediately

    SDL_Window *window = SDL_CreateWindow("Server", 640, 480, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    TTF_Font *font = TTF_OpenFont(GetResourcePath("UnifontExMono.ttf").c_str(), 12);
    if (!font)
        throw std::runtime_error("Failed to load font: " + std::string(SDL_GetError()));

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            if (event.type == SDL_EVENT_QUIT)
                running = false;

        // Lock once for the entire render pass so the player list and bgColor
        // can't change mid-frame.
        std::scoped_lock lock(stateMutex);

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        for (const auto &p : players)
            RenderPlayer(renderer, font, p);

        // Server address overlay
        std::string address = server.GetServerAddress();
        SDL_Color white{255, 255, 255, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(font, address.c_str(), address.length(), white);
        if (surf)
        {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            if (tex)
            {
                SDL_FRect tr{10, 10, surf->w, surf->h};
                SDL_RenderTexture(renderer, tex, nullptr, &tr);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(surf);
        }

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    server.Shutdown();
}

// ============================================================================
void RunClient(const std::string &serverAddress, uint16_t serverPort)
{
    // stateMutex guards: player, otherPlayers
    // All ASIO callbacks (client ASIO thread) and the SDL game loop
    // (main thread) must hold this lock before touching either.
    std::mutex stateMutex;
    Player player;
    std::vector<Player> otherPlayers;

    nxs::Client<MyMessageType> client;
    client.Connect(serverAddress, serverPort);

    client.AddMessageHandler(
        MyMessageType::EchoPlayer,
        [&](nxs::Message<MyMessageType> &msg)
        {
            std::scoped_lock lock(stateMutex);
            msg >> player;
        });

    client.AddMessageHandler(
        MyMessageType::PlayerJoined,
        [&](nxs::Message<MyMessageType> &msg)
        {
            Player newPlayer;
            msg >> newPlayer;
            std::scoped_lock lock(stateMutex);
            otherPlayers.push_back(newPlayer);
        });

    client.AddMessageHandler(
        MyMessageType::GetPlayers,
        [&](nxs::Message<MyMessageType> &msg)
        {
            std::scoped_lock lock(stateMutex);
            otherPlayers.clear();
            while (msg.m_Header.size != 0)
            {
                SDL_FPoint pos;
                uint32_t id;
                SDL_Color color;
                msg >> color.a >> color.b >> color.g >> color.r >> id >> pos.y >> pos.x;
                if (id != player.ID)
                    otherPlayers.push_back({pos, id, color});
            }
        });

    client.AddMessageHandler(
        MyMessageType::MoveBroadcast,
        [&](nxs::Message<MyMessageType> &msg)
        {
            uint32_t id;
            SDL_FPoint pos;
            msg >> pos.y >> pos.x >> id;

            std::scoped_lock lock(stateMutex);
            auto it = std::find_if(otherPlayers.begin(), otherPlayers.end(),
                                   [id](const Player &p)
                                   { return p.ID == id; });
            if (it != otherPlayers.end())
                it->pos = pos;
        });

    client.AddMessageHandler(
        MyMessageType::MoveAccepted,
        [&](nxs::Message<MyMessageType> &msg)
        {
            SDL_FPoint accepted;
            msg >> accepted.y >> accepted.x;
            std::scoped_lock lock(stateMutex);
            player.pos = accepted;
        });

    client.AddMessageHandler(
        MyMessageType::PlayerDisconnected,
        [&](nxs::Message<MyMessageType> &msg)
        {
            uint32_t id;
            msg >> id;
            std::scoped_lock lock(stateMutex);
            std::erase_if(otherPlayers, [id](const Player &p)
                          { return p.ID == id; });
        });

    SDL_Window *window = SDL_CreateWindow("Client", 640, 480, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    TTF_Font *font = TTF_OpenFont(GetResourcePath("UnifontExMono.ttf").c_str(), 12);
    if (!font)
        throw std::runtime_error("Failed to load font: " + std::string(SDL_GetError()));

    bool running = true;
    while (running)
    {
        Uint32 frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event))
            if (event.type == SDL_EVENT_QUIT)
                running = false;

        // Read input and compute movement outside the lock — no shared state
        // involved until we need player.pos.
        const bool *keys = SDL_GetKeyboardState(nullptr);

        {
            // Snapshot current position, compute new position, send and update
            // all under the lock so there's no window between reading player.pos
            // and writing it back.
            std::scoped_lock lock(stateMutex);

            SDL_FPoint newPos = player.pos;
            if (keys[SDL_SCANCODE_UP])
                newPos.y -= 1;
            else if (keys[SDL_SCANCODE_DOWN])
                newPos.y += 1;
            else if (keys[SDL_SCANCODE_LEFT])
                newPos.x -= 1;
            else if (keys[SDL_SCANCODE_RIGHT])
                newPos.x += 1;

            if (newPos.x != player.pos.x || newPos.y != player.pos.y)
            {
                nxs::Message<MyMessageType> msg{MyMessageType::Move};
                msg << newPos.x << newPos.y;
                client.Send(msg);
                player.pos = newPos; // optimistic local update
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
        SDL_RenderClear(renderer);

        {
            // Lock once for the entire render pass.
            std::scoped_lock lock(stateMutex);
            for (const auto &p : otherPlayers)
                RenderPlayer(renderer, font, p);
            RenderPlayer(renderer, font, player);
        }

        SDL_RenderPresent(renderer);

        Uint32 elapsed = SDL_GetTicks() - frameStart;
        if (elapsed < 1000u / 60u)
            SDL_Delay((1000u / 60u) - elapsed);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    client.Disconnect();
    SDL_Delay(100);
}

// ============================================================================
uint16_t ParsePort(const std::string &portStr)
{
    try
    {
        int port = std::stoi(portStr);
        if (port < 1 || port > 65535)
            throw std::out_of_range("Port must be in range 1–65535.");
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
                  << " --server [port] | --client <ip> <port>\n";
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
                std::cerr << "Usage: " << argv[0] << " --client <ip> <port>\n";
                return 1;
            }
            RunClient(argv[2], ParsePort(argv[3]));
        }
        else
        {
            std::cerr << "Invalid mode. Use --server or --client\n";
            return 1;
        }

        nxs::Nexus::Shutdown();
        ShutdownSDL();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        // Shut down ASIO threads before SDL so in-flight async operations
        // don't fire callbacks into already-destroyed SDL/game state.
        nxs::Nexus::Shutdown();
        ShutdownSDL();
        return 1;
    }

    return 0;
}
