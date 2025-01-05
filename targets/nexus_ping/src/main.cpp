#include <iostream>
#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <nexus/message.h>

enum class MyMessageType : uint32_t
{
    Ping,
    Pong,
};

int main(int argc, char* argv[])
{
    nxs::Nexus::Init();

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    if (std::string(argv[1]) == "--server")
    {
        nxs::Server<MyMessageType> server{};

        // Event handler for new connections
        server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect,
            [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection)
            {
                std::cout << "[SERVER] " << "Client [" << connection->GetID() << "] connected!\n";
            });
        server.AddMessageHandler(MyMessageType::Ping,
            [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection, nxs::Message<MyMessageType>& message)
            {
                std::cout << "[SERVER] " << "Received Ping message from client [" << connection->GetID() << "]\n";
                nxs::Message<MyMessageType> response{ MyMessageType::Pong };
                connection->Send(response);
            });

        server.Run(true); // Blocks the main thread
    }
    else if (std::string(argv[1]) == "--client")
    {
        nxs::Client<MyMessageType> client;
        client.Connect("localhost", 60000);
        std::chrono::time_point<std::chrono::system_clock> lastPingTime;
        std::chrono::time_point<std::chrono::system_clock> currentTime;
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnConnect,
            [&]()
            {
                std::cout << "[" << client.GetID() << "] " << "Connected to server\n";
                nxs::Message<MyMessageType> message{ MyMessageType::Ping };
                client.Send(message);
                lastPingTime = std::chrono::system_clock::now();

            });
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnDisconnect,
            [&]()
            {
                std::cout << "[" << client.GetID() << "] " << "Disconnected from server\n";
            });
        client.AddMessageHandler(MyMessageType::Pong,
            [&](nxs::Message<MyMessageType>& message)
            {
                currentTime = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsedSeconds = currentTime - lastPingTime;
                std::cout << "[" << client.GetID() << "] " << "Received Pong message\n";
                std::cout << "[" << client.GetID() << "] " << "Round trip time: " << elapsedSeconds.count() << "s\n";
            });
        while (true)
        {
        }
    }
    else
    {
        std::cerr << "Invalid mode. Use --server or --client\n";
        return 1;
    }

    nxs::Nexus::Shutdown();
    return 0;
}