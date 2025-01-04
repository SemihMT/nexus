#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <iostream>

enum class MyMessageType : uint32_t
{
    Connect,
    Disconnect,
};
void DisconnectClientImmediately(std::shared_ptr<nxs::Connection<MyMessageType>> connection)
{
    connection->Disconnect();
}

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
        std::cout << "Starting server...\n";
        // Create the server on port 60000
        nxs::Server<MyMessageType> s{60000};
        s.AddEventHandler(nxs::Server<MyMessageType>::ServerEvent::OnConnect, DisconnectClientImmediately);
    }
    else if(std::string(argv[1]) == "--client")
    {
        std::cout << "Starting client...\n";
        // Create the client
        nxs::Client<MyMessageType> client;
        client.Connect("127.0.0.1", 60000);
    }
    else
    {
        std::cerr << "Invalid mode. Use --server or --client\n";
        return 1;
    }
    
    nxs::Nexus::Shutdown();
    return 0;
}