#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <iostream>

enum class MyMessageType : uint32_t
{
    Connect,
    Disconnect,
};
int main(int argc, char *argv[])
{
    nxs::Nexus::Init();

    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }
    if(std::string(argv[1]) == "--server")
    {
        nxs::Server<MyMessageType> server{};

        // Event handler for new connections
        server.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect,
                               [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection)
                               {
                                   std::cout << "[SERVER] " << "Client [" << connection->GetID() <<"] connected!\n";
                                   // Disconnect the client immediately
                                   connection->Disconnect();
                               });

        server.Run(true); // Blocks the main thread
    }
    else if(std::string(argv[1]) == "--client")
    {
        nxs::Client<MyMessageType> client;
        client.Connect("localhost", 60000);
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnConnect,
                               [&]()
                               {
                                   std::cout << "[" << client.GetID() << "] " << "Connected to server\n";
                               });
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnDisconnect,
                               [&]()
                               {
                                   std::cout << "[" << client.GetID() << "] " << "Disconnected from server\n";
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