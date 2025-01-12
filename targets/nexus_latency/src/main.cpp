#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <nexus/message.h>
#include <iostream>
#include <numeric>

enum class MyMessageType : uint32_t
{
    Message,
    Reply,
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
        int numMessages = 50;
        std::vector<long long> roundTripTimes;
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

                                   client.Send(message);
                                   messageCount++;
                                   start = std::chrono::high_resolution_clock::now();
                               });
        client.AddEventHandler(nxs::Client<MyMessageType>::ClientEvent::OnDisconnect,
                               [&]()
                               {
                                   std::cout << "[CLIENT] [" << client.GetID() << "] " << "Disconnected from server\n";
                               });
        client.AddMessageHandler(MyMessageType::Reply,
                                 [&](nxs::Message<MyMessageType> &message)
                                 {
                                     end = std::chrono::high_resolution_clock::now();
                                     // Calculate round trip time
                                     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                                     std::cout << "[CLIENT] " << "Received reply from server\n";
                                     roundTripTimes.push_back(duration.count());
                                     if (messageCount < numMessages)
                                     {
                                         // Create a 1KB message
                                         nxs::Message<MyMessageType> message{MyMessageType::Message};
                                         message.m_Data.resize(1024);
                                         message.m_Header.size = message.m_Data.size();
                                         std::fill(message.m_Data.begin(), message.m_Data.end(), std::byte{'A'});

                                         start = std::chrono::high_resolution_clock::now();
                                         client.Send(message);
                                         messageCount++;
                                     }
                                     else
                                     {
                                         // print all round trip times
                                         std::cout << "Round trip times:\n";
                                         for (auto time : roundTripTimes)
                                         {
                                             std::cout << time << "\n";
                                         }
                                         // remove biggest and smallest round trip times
                                         roundTripTimes.erase(std::max_element(roundTripTimes.begin(), roundTripTimes.end()));
                                         roundTripTimes.erase(std::min_element(roundTripTimes.begin(), roundTripTimes.end()));
                                         // calculate average round trip time
                                         long long sum = std::accumulate(roundTripTimes.begin(), roundTripTimes.end(), 0);
                                         long long average = sum / roundTripTimes.size();
                                         std::cout << "Average round trip time: " << average << "\n";
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