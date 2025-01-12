#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <nexus/message.h>
#include <iostream>

enum class MyMessageType : uint32_t
{
    Message,
    Reply
};

int main(int argc, char *argv[])
{
    nxs::Nexus::Init();
    std::chrono::high_resolution_clock::time_point start, end;

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
        server.AddMessageHandler(MyMessageType::Message,
                                 [&](std::shared_ptr<nxs::Connection<MyMessageType>> connection, nxs::Message<MyMessageType> &message)
                                 {
                                     std::cout << "[SERVER] " << "Received message from client [" << connection->GetID() << "]\n";
                                     nxs::Message<MyMessageType> reply{MyMessageType::Reply};
                                     reply.m_Data.resize(1024);
                                     reply.m_Header.size = reply.m_Data.size();
                                     std::fill(reply.m_Data.begin(), reply.m_Data.end(), std::byte{'A'});
                                     connection->Send(reply);
                                 });
        server.Run(true); // Blocks the main thread
    }
    else if (std::string(argv[1]) == "--client")
    {
        int messageCount = 0;
        int numMessages = 1024;
        int testAmount = 50;
        int testCount = 0;
        size_t sentMessageSize;
        std::vector<double> througputValues;
        nxs::Client<MyMessageType> client;
        client.Connect("localhost", 60000);
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnConnect,
                               [&]()
                               {
                                   std::cout << "[CLIENT] [" << client.GetID() << "] " << "Connected to server\n";

                                   // Create a 1KB message
                                   nxs::Message<MyMessageType> message{MyMessageType::Message};
                                   message.m_Data.resize(1024);
                                   message.m_Header.size = message.m_Data.size();
                                   std::fill(message.m_Data.begin(), message.m_Data.end(), std::byte{'A'});
                                    sentMessageSize = message.Size();
                                   start = std::chrono::high_resolution_clock::now();
                                   for (int i = 0; i < numMessages; i++)
                                   {
                                       client.Send(message);
                                   }
                                   ++testCount;
                               });
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnDisconnect,
                               [&]()
                               {
                                   std::cout << "[CLIENT] [" << client.GetID() << "] " << "Disconnected from server\n";
                               });
        client.AddMessageHandler(MyMessageType::Reply,
                                 [&](nxs::Message<MyMessageType> &message)
                                 {
                                    ++messageCount;
                                    if (messageCount == numMessages)
                                    {
                                        if(testCount != testAmount + 1)
                                        {
end = std::chrono::high_resolution_clock::now();
                                        // Calculate round trip time
                                        std::chrono::duration<double> duration = end - start;
                                        auto totalDataTransferred = static_cast<double>(numMessages * sentMessageSize)  / (1024.0 * 1024);
                                        auto throughput = totalDataTransferred / duration.count();
                                        througputValues.push_back(throughput);
                                        std::cout << "Messages sent & received: " << numMessages << "\n";
                                        std::cout << "Sending " << numMessages << " messages took " << duration.count() << " seconds\n";
                                        std::cout << "Single message size (header + body): " << sentMessageSize << "\n";
                                        std::cout << "Total data transferred: " << totalDataTransferred << " MB\n";
                                        std::cout << "Throughput: " << throughput << " MB/s\n";

                                        messageCount = 0;
                                        nxs::Message<MyMessageType> message{MyMessageType::Message};
                                        message.m_Data.resize(1024);
                                        message.m_Header.size = message.m_Data.size();
                                        std::fill(message.m_Data.begin(), message.m_Data.end(), std::byte{'A'});
                                        sentMessageSize = message.Size();
                                        start = std::chrono::high_resolution_clock::now();
                                        for (int i = 0; i < numMessages; i++)
                                        {
                                            client.Send(message);
                                        }
                                        ++testCount;
                                        }
                                        else
                                        {
                                            std::cout << "All throughput values: \n";
                                            for(const auto& t : througputValues)
                                            {
                                                std::cout << t << " MB/s\n";
                                            }
                                        }
                                        
                                    }
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