#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring> // For memcpy

int main(int argc, char *argv[])
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to initialize ENet.\n";
        return 1;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        enet_deinitialize();
        return 1;
    }

    if (std::string(argv[1]) == "--server")
    {
        ENetAddress address;
        ENetHost *server;

        address.host = ENET_HOST_ANY;
        address.port = 60000;

        server = enet_host_create(&address, 32, 2, 0, 0);
        if (!server)
        {
            std::cerr << "Failed to create server host.\n";
            enet_deinitialize();
            return 1;
        }

        std::cout << "[SERVER] Running on port 60000...\n";

        while (true)
        {
            ENetEvent event;
            while (enet_host_service(server, &event, 1000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    {std::cout << "[SERVER] Client connected from "
                              << (event.peer->address.host & 0xFF) << "."
                              << ((event.peer->address.host >> 8) & 0xFF) << "."
                              << ((event.peer->address.host >> 16) & 0xFF) << "."
                              << ((event.peer->address.host >> 24) & 0xFF) << ":"
                              << event.peer->address.port << "\n";}
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    {std::cout << "[SERVER] Received message of size " << event.packet->dataLength << " bytes.\n";

                    // Create a reply packet
                    ENetPacket *replyPacket = enet_packet_create("Reply", 1024, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, replyPacket);
                    std::cout << "[SERVER] Sent reply.\n";

                    enet_packet_destroy(event.packet);}
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                   { std::cout << "[SERVER] Client disconnected.\n";
                    break;}

                default:
                    break;
                }
            }
        }

        enet_host_destroy(server);
    }
    else if (std::string(argv[1]) == "--client")
    {
        ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!client)
        {
            std::cerr << "Failed to create client host.\n";
            enet_deinitialize();
            return 1;
        }

        ENetAddress address;
        ENetPeer *peer;

        enet_address_set_host(&address, "localhost");
        address.port = 60000;

        peer = enet_host_connect(client, &address, 2, 0);
        if (!peer)
        {
            std::cerr << "Failed to connect to server.\n";
            enet_host_destroy(client);
            enet_deinitialize();
            return 1;
        }

        std::cout << "[CLIENT] Connecting to server...\n";

        ENetEvent event;
        if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            std::cout << "[CLIENT] Connected to server.\n";

            int messageCount = 0;
            int numMessages = 50;
            std::vector<long long> roundTripTimes;
            std::chrono::high_resolution_clock::time_point start, end;

            // Create a 1KB message
            char messageData[1024];
            std::memset(messageData, 'A', sizeof(messageData));

            while (messageCount < numMessages)
            {
                start = std::chrono::high_resolution_clock::now();

                ENetPacket *packet = enet_packet_create(messageData, sizeof(messageData), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, packet);

                while (true)
                {
                    if (enet_host_service(client, &event, 5000) > 0)
                    {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE)
                        {
                            end = std::chrono::high_resolution_clock::now();

                            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                            roundTripTimes.push_back(duration.count());

                            std::cout << "[CLIENT] Received reply of size " << event.packet->dataLength << " bytes.\n";
                            std::cout << "Round trip time: " << duration.count() << " us\n";

                            enet_packet_destroy(event.packet);
                            break;
                        }
                    }
                }

                messageCount++;
            }

            // Print round trip times
            std::cout << "Round trip times:\n";
            for (auto time : roundTripTimes)
            {
                std::cout << time << "\n";
            }

            roundTripTimes.erase(std::max_element(roundTripTimes.begin(), roundTripTimes.end()));
            roundTripTimes.erase(std::min_element(roundTripTimes.begin(), roundTripTimes.end()));
            long long sum = std::accumulate(roundTripTimes.begin(), roundTripTimes.end(), 0LL);
            long long average = sum / roundTripTimes.size();
            std::cout << "Average round trip time: " << average << " us\n";
        }
        else
        {
            std::cerr << "[CLIENT] Connection to server failed.\n";
        }

        enet_host_destroy(client);
    }
    else
    {
        std::cerr << "Invalid mode. Use --server or --client\n";
        enet_deinitialize();
        return 1;
    }

    enet_deinitialize();
    return 0;
}
