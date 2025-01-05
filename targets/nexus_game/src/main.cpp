// #include "nexus.h"
// #include <SDL.h>
// #include <iostream>
// #include "nxsg_window.h"
// #include "nxsg_renderer.h"

// bool HandleEvents()
// {
//     SDL_Event event;
//     while (SDL_PollEvent(&event))
//     {
//         switch (event.type)
//         {
//         case SDL_QUIT:
//             return true;
//             break;
//         }
//     }
//     return false;
// }

// int main(int argc, char *argv[])
// {

//     // returns zero on success else non-zero
//     if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
//     {
//         printf("error initializing SDL: %s\n", SDL_GetError());
//     }

//     nxsg::window window{};
//     nxsg::renderer renderer{window};

//     bool shouldClose{};
//     while (!shouldClose)
//     {
//         // Process input
//         shouldClose = HandleEvents();

//         // Update the game here:
//         // TODO: update function

//         renderer.SetDrawColor({128,0, 0, 255});
//         renderer.Clear();
//         // Do rendering here

//         renderer.Present();
//     }

//     SDL_Quit();
//     return 0;
// }

// #include <iostream>
// #include <asio.hpp>

// using asio::ip::tcp;

// class session : public std::enable_shared_from_this<session>
// {
// public:
//     session(tcp::socket socket) : m_socket(std::move(socket)) {}

//     void run()
//     {
//         wait_for_request();
//     }

// private:
//     void wait_for_request()
//     {
//         auto self(shared_from_this());
//         asio::async_read_until(m_socket, m_buffer, "\0",
//                                [this, self](asio::error_code ec, std::size_t /*length*/)
//                                {
//                                    if (!ec)
//                                    {
//                                        std::string data{
//                                            std::istreambuf_iterator<char>(&m_buffer),
//                                            std::istreambuf_iterator<char>()};
//                                        std::cout << data << std::endl;
//                                        wait_for_request();
//                                    }
//                                    else
//                                    {
//                                        std::cout << "error: " << ec << std::endl;
//                                        ;
//                                    }
//                                });
//     }

// private:
//     tcp::socket m_socket;
//     asio::streambuf m_buffer;
// };

// class server
// {
// public:
//     server(asio::io_context &io_context, short port)
//         : m_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
//     {
//         do_accept();
//     }

// private:
//     void do_accept()
//     {
//         m_acceptor.async_accept([this](asio::error_code ec, tcp::socket socket)
//                                 {
//             if (!ec) {
//                 std::cout << "creating session on: "
//                     << socket.remote_endpoint().address().to_string()
//                     << ":" << socket.remote_endpoint().port() << '\n';

//                 std::make_shared<session>(std::move(socket))->run();
//             } else {
//                 std::cout << "error: " << ec.message() << std::endl;
//             }
//             do_accept(); });
//     }

// private:
//     tcp::acceptor m_acceptor;
// };

// class Client
// {
// public:
//     Client(const std::string &host, const std::string &port)
//         : io_context_(), socket_(io_context_), resolver_(io_context_), host_(host), port_(port) {}

//     void connect()
//     {
//         asio::connect(socket_, resolver_.resolve(host_, port_));
//         std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
//     }

//     void sendData(const std::string &data)
//     {
//         auto result = asio::write(socket_, asio::buffer(data));
//         std::cout << "Data sent: " << data.length() << '/' << result << std::endl;
//     }
//     void interact()
//     {
//         std::string userInput;
//         std::cout << "Enter messages to send (type 'exit' to quit):" << std::endl;
//         while (true)
//         {
//             std::getline(std::cin, userInput); // Read user input from console
//             if (userInput == "exit")
//             {
//                 std::cout << "Exiting..." << std::endl;
//                 break;
//             }
//             sendData(userInput);
//         }
//     }
//     void closeConnection()
//     {
//         asio::error_code ec;
//         socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
//         if (ec)
//         {
//             std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
//         }
//         socket_.close(ec);
//         if (ec)
//         {
//             std::cerr << "Error closing socket: " << ec.message() << std::endl;
//         }
//         std::cout << "Connection closed." << std::endl;
//     }

// private:
//     asio::io_context io_context_;
//     asio::ip::tcp::socket socket_;
//     asio::ip::tcp::resolver resolver_;
//     std::string host_;
//     std::string port_;
// };

// int main(int argc, char *argv[])
// {
//     if (argc < 2)
//     {
//         std::cout << "Wrong usage! Example usage: ./Nexus_Game.exe s\n";
//         return 1;
//     }
//     std::string mode = argv[1]; // Convert argv[1] to std::string for comparison

//     if (mode == "s")
//     {
//         std::cout << "Started as server!" << std::endl;
//         asio::io_context io_context;
//         server s(io_context, 25000); // Assuming a `server` class is defined
//         io_context.run();
//     }
//     else if (mode == "c")
//     {
//         std::cout << "Started as client!" << std::endl;
//         Client c{"127.0.0.1", "25000"}; // Assuming a `Client` class is defined
//         c.connect();
//         c.interact(); // Interact with the user to send messages
//         c.closeConnection();
//     }
//     else
//     {
//         std::cout << "Invalid argument! Use 's' for server or 'c' for client.\n";
//         return 1;
//     }

//     return 0;
// }

#include <SDL.h>
#include "SDL_ttf.h"
#include "nexus/nexus.h"
#include "nexus/server.h"
// I should probably make a central nexus_game.h file that includes all the necessary headers
#include <nexus_game/nxsg_window.h>
#include <nexus_game/nxsg_renderer.h>
#include <nexus_game/nxsg_text.h>
#include <nexus/client.h>

// enum class MyMessageType : uint32_t
// {
//     FirstMessage,
//     Ping,
//     Pong,
// };
// int main(int argc, char *argv[])
// {
//     // Initialize the Nexus library
//     nxs::Nexus::Init();

//     // returns zero on success else non-zero
//     if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
//     {
//         printf("error initializing SDL: %s\n", SDL_GetError());
//     }

//     if (TTF_Init() == -1)
//     {
//         printf("error initializing SDL_ttf: %s\n", TTF_GetError());
//     }

//     nxsg::window window{};
//     nxsg::renderer renderer{window};
//     SDL_Color color{128, 0, 0, 255};
//     SDL_Point position{0, 0};
//     std::string message = "Hello";
//     nxsg::Text text{renderer, "../../resources/UnifontExMono.ttf", 12};
//     nxsg::Text connectedClientsText{renderer, "../../resources/UnifontExMono.ttf", 12};
//     int connectedClients{0};
//     // Starts server on port 60000
//     nxs::Server<MyMessageType> s{60000};
//     s.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection)
//                       { 
//         std::cout << "Client connected!\n";
//         ++connectedClients;
//         connectedClientsText.markDirty();
//         color = {0,255,0,255}; });
//     s.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnDisconnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>>)
//                       { 
//         std::cout << "Client disconnected!\n";
//         --connectedClients;
//         connectedClientsText.markDirty();
//         if(connectedClients == 0)
//             color = {255,0,0,255}; });

//     s.AddMessageHandler(MyMessageType::Ping, [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection, nxs::Message<MyMessageType> &message)
//                         {
//                             std::cout << "Received message: " << message << std::endl;
//                             // Convert message.m_Data to std::string
//                             std::string contents;
//                             message >> contents;
//                             std::cout << "Contents: " << contents << std::endl;
//                             // Echo message back to client
//                             // connection->Send(message);
//                         });
//     s.AddMessageHandler(MyMessageType::Pong, [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection, nxs::Message<MyMessageType> &message)
//                         {
//                             std::cout << "Received message: " << message << "From connection with ID: " << connection->GetID() <<  std::endl;
//                             // Convert message.m_Data to std::string
//                             std::string contents;
//                             message >> contents;
//                             std::cout << "Contents: " << contents << std::endl;
//                             // Echo message back to client
//                             // connection->Send(message);
//                         });
//     s.Run();

//     nxs::Client<MyMessageType> c;
//     c.AddMessageHandler(MyMessageType::Ping, [&](nxs::Message<MyMessageType> &message)
//                         {
//         std::cout << "Received message: " << message << std::endl;
//         // Convert message.m_Data to std::string
//         std::string contents;
//         message >> contents;
//         std::cout << "Contents: " << contents << std::endl;

//         nxs::Message<MyMessageType> reply{MyMessageType::Pong, c.GetID()};
//         reply << std::string{"Hello from client!"};
//         c.Send(reply); });
//     c.Connect("127.0.0.1", 60000);

//     SDL_StartTextInput();
//     bool shouldClose{false};

//     while (!shouldClose)
//     {
//         SDL_Event event;
//         while (SDL_PollEvent(&event))
//         {
//             switch (event.type)
//             {
//             case SDL_QUIT:
//                 shouldClose = true;
//                 break;
//             case SDL_KEYUP:
//                 // Handle backspace
//                 if (event.key.keysym.sym == SDLK_BACKSPACE && message.length() > 0)
//                 {
//                     // lop off character
//                     message.pop_back();
//                     text.markDirty();
//                 }
//                 // Handle copy
//                 else if (event.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL)
//                 {
//                     SDL_SetClipboardText(message.c_str());
//                 }
//                 // Handle paste
//                 else if (event.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL)
//                 {
//                     // Copy text from temporary buffer
//                     char *tempText = SDL_GetClipboardText();
//                     message = tempText;
//                     SDL_free(tempText);
//                     text.markDirty();
//                 }
//                 else if (event.key.keysym.sym == SDLK_RETURN)
//                 {
//                     text.markDirty();
//                     nxs::Message<MyMessageType> message{MyMessageType::Ping};
//                     message << std::string{"Hello from server!"};
//                     s.Broadcast(message);
//                     // message += '\n';
//                 }
//                 break;

//             // Special text input event
//             case SDL_TEXTINPUT:
//                 // Check if we're not copying or pasting
//                 if (!(SDL_GetModState() & KMOD_CTRL && (event.text.text[0] == 'c' || event.text.text[0] == 'C' || event.text.text[0] == 'v' || event.text.text[0] == 'V')))
//                 {
//                     text.markDirty();
//                     // Append character
//                     message += event.text.text;
//                 }
//                 break;

//             default:
//                 break;
//             }
//         }

//         // Update the game here:
//         // TODO: update function
//         renderer.SetDrawColor(color);
//         renderer.Clear();
//         // Do rendering here
//         // Render text
//         text.setText(message, {255, 255, 255, 255});
//         connectedClientsText.setText("Connected clients: " + std::to_string(connectedClients), {255, 255, 255, 255});

//         text.render(position.x, position.y);
//         connectedClientsText.render(window.width() - connectedClientsText.width(), 0);
//         renderer.Present();
//     }
//     nxs::Nexus::Shutdown();
//     SDL_StopTextInput();
//     TTF_Quit();
//     SDL_Quit();

//     return 0;
// }
std::mutex textMutex;
std::vector<std::string> g_TextToRender;
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    if(!surface) {
        throw std::runtime_error("Error rendering text: " + std::string(TTF_GetError()));
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(!texture) {
        throw std::runtime_error("Error creating texture from surface: " + std::string(SDL_GetError()));
    }
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    if(SDL_RenderCopy(renderer, texture, nullptr, &dstRect) != 0) {
        throw std::runtime_error("Error rendering text: " + std::string(SDL_GetError()));
    }
    //SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
// Safely add text to the render queue
void AddTextToRender(const std::string& text) {
    std::lock_guard<std::mutex> lock(textMutex);
    g_TextToRender.push_back(text);
}
// Safely render text in the render loop
void RenderTextQueue(SDL_Renderer* renderer, TTF_Font* font) {
    int yOffset = 60;
    std::lock_guard<std::mutex> lock(textMutex);
    for (const auto& text : g_TextToRender) {
        RenderText(renderer, font, text, 20, yOffset, {255, 255, 255, 255});
        yOffset += 30;
    }
}
enum class MyMessageType : uint32_t {
    FirstMessage,
    Ping,
    Pong,
};

void InitSDL() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        throw std::runtime_error("Error initializing SDL: " + std::string(SDL_GetError()));
    }

    if (TTF_Init() == -1) {
        throw std::runtime_error("Error initializing SDL_ttf: " + std::string(TTF_GetError()));
    }
}

void ShutdownSDL() {
    TTF_Quit();
    SDL_Quit();
}

void RunServer() {
    int connectedClients = 0;
    SDL_Color bgColor = {255, 0, 0, 255}; // Red for no connections

    nxs::Server<MyMessageType> server{60000};
    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn) {
        std::cout << "Client connected!\n";
        connectedClients++;
        bgColor = {0, 255, 0, 255}; // Green for active connections
    });
    server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnDisconnect, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn) {
        std::cout << "Client disconnected!\n";
        connectedClients--;
        if (connectedClients == 0) {
            bgColor = {255, 0, 0, 255}; // Red for no connections
        }
    });
    server.AddMessageHandler(MyMessageType::Ping, [&](std::shared_ptr<nxs::Connection<MyMessageType>> conn, nxs::Message<MyMessageType>& msg) {
        std::string contents;
        msg >> contents;
        // Push message to render
        AddTextToRender("Received: " + contents + " From: " + std::to_string(conn->GetID()));
        std::cout << "Received: " << contents << " From: " << conn->GetID() << "\n";

    });

    server.Run();

    SDL_Window* window = SDL_CreateWindow("Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("resources/UnifontExMono.ttf", 12);
    if (!font) {
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
    }
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        // Render connected clients text
        std::string clientText = "Connected clients: " + std::to_string(connectedClients);
        RenderText(renderer, font, clientText, 0, 0, {255, 255, 255, 255});
        RenderTextQueue(renderer, font);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    server.Shutdown();
}

void RunClient(const std::string& serverAddress, uint16_t serverPort) {
    nxs::Client<MyMessageType> client;
    client.AddMessageHandler(MyMessageType::Pong, [&](nxs::Message<MyMessageType>& message) {
        std::string contents;
        message >> contents;
        std::cout << "Received: " << contents << "\n";
    });

    client.Connect(serverAddress, serverPort);

    SDL_Window* window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_RETURN) {
                nxs::Message<MyMessageType> msg{MyMessageType::Ping};
                msg << std::string{"Hello from client!"};
                client.Send(msg);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255); // Dark blue background
        SDL_RenderClear(renderer);

        // Render text
        RenderText(renderer, TTF_OpenFont("resources/UnifontExMono.ttf", 12), "Press Enter to send message", 0, 0, {255, 255, 255, 255});


        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    client.Disconnect();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    try {
        nxs::Nexus::Init();
        InitSDL();

        std::string mode = argv[1];
        if (mode == "--server") {
            RunServer();
        } else if (mode == "--client") {
            RunClient("127.0.0.1", 60000);
        } else {
            std::cerr << "Invalid mode. Use --server or --client\n";
            return 1;
        }

        ShutdownSDL();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        ShutdownSDL();
        return 1;
    }

    return 0;
}