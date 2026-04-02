#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <iostream>

enum class MyMessageType : uint32_t
{
    ChatMessage
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
            .OnConnect([](auto conn)
                       { std::cout << "[SERVER] Client [" << conn->GetID() << "] connected.\n"; })
            .OnMessage(MyMessageType::ChatMessage, [&server](auto conn, auto &msg)
                       {
                uint32_t    recipient;
                std::string text;
                msg >> recipient >> text;

                std::cout << "[SERVER] [" << conn->GetID() << "] -> ["
                          << recipient << "]: " << text << "\n";

                nxs::Message<MyMessageType> fwd{MyMessageType::ChatMessage, conn->GetID()};
                fwd << text;
                server.SendToClient(recipient, fwd); })
            .RunBlocking();
    }
    else if (std::string(argv[1]) == "--client")
    {
        nxs::Client<MyMessageType> client;

        client
            .OnConnect([&]()
                       {
                std::cout << "[" << client.GetID() << "] Connected.\n";

                // Second client to connect (ID 1002) sends a message to the first (1001).
                if (client.GetID() == 1002)
                {
                    nxs::Message<MyMessageType> msg{MyMessageType::ChatMessage};
                    msg << std::string{"Hello, client 1001!"} << uint32_t{1001};
                    client.Send(msg);
                } })
            .OnMessage(MyMessageType::ChatMessage, [&](auto &msg)
                       {
                std::string text;
                msg >> text;
                std::cout << "[" << client.GetID() << "] Message from ["
                          << msg.m_Header.owner << "]: " << text << "\n"; })
            .ConnectTo("localhost", 60000);

        while (true)
        {
        }
    }

    nxs::Nexus::Shutdown();
    return 0;
}
