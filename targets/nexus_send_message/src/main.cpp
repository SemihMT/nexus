#include <iostream>
#include <nexus/nexus.h>
#include <nexus/client.h>
#include <nexus/message.h>
#include <nexus/server.h>

enum class MyMessageType : uint32_t
{
    ChatMessage
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
        server.AddMessageHandler(MyMessageType::ChatMessage,
            [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection, nxs::Message<MyMessageType>& message)
            {
                uint32_t messageRecipient;
                std::string messageContent;
                message >> messageRecipient >> messageContent;

                std::cout << "[SERVER] " << "message from client [" << connection->GetID() << "] for client [" << messageRecipient << "]: " << messageContent << "\n";

                nxs::Message<MyMessageType> response{ MyMessageType::ChatMessage, connection->GetID() };
                response << messageContent;
                server.SendToClient(messageRecipient, response);
            });

        server.Run(true); // Blocks the main thread
    }
    else if (std::string(argv[1]) == "--client")
    {
        nxs::Client<MyMessageType> client;
        client.Connect("localhost", 60000);
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnConnect,
            [&]()
            {
                std::cout << "[" << client.GetID() << "] " << "Connected to server\n";
                if(client.GetID() == 1002)
                {
                    nxs::Message<MyMessageType> message{ MyMessageType::ChatMessage };
                    message << std::string{"Hello, client 1001!"} << static_cast<uint32_t>(1001);
                    client.Send(message);
                }

            });
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnDisconnect,
            [&]()
            {
                std::cout << "[" << client.GetID() << "] " << "Disconnected from server\n";
            });
        client.AddMessageHandler(MyMessageType::ChatMessage,
            [&](nxs::Message<MyMessageType>& message)
            {
                uint32_t messageSender = message.m_Header.owner;
                std::string messageContent;
                message >> messageContent;

                std::cout << "[" << client.GetID() << "] " << "message from client [" << messageSender << "]: " << messageContent << "\n";
            });
        while (true)
        {
        }
        
    }
}