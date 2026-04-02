#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <iostream>

enum class MyMessageType : uint32_t
{
    Placeholder
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    nxs::Nexus::Init();

    if (std::string(argv[1]) == "--server")
    {
        nxs::Server<MyMessageType> server;

        server
            .OnConnect([&](auto conn)
                       {
                std::cout << "[SERVER] Client [" << conn->GetID() << "] connected. Disconnecting.\n";
                conn->Disconnect(); })
            .OnDisconnect([](auto conn)
                          { std::cout << "[SERVER] Client [" << conn->GetID() << "] disconnected.\n"; })
            .RunBlocking();
    }
    else if (std::string(argv[1]) == "--client")
    {
        nxs::Client<MyMessageType> client;

        client
            .OnConnect([&]()
                       { std::cout << "[" << client.GetID() << "] Connected.\n"; })
            .OnDisconnect([&]()
                          { std::cout << "[" << client.GetID() << "] Disconnected.\n"; })
            .ConnectTo("localhost", 60000);

        while (true)
        {
        }
    }

    nxs::Nexus::Shutdown();
    return 0;
}
