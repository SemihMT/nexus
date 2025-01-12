#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>

const size_t MESSAGE_SIZE = 1024;
const int NUM_MESSAGES = 1024;
const int TEST_AMOUNT = 50;

void server()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "[SERVER] Failed to initialize ENet.\n";
        return;
    }

    ENetAddress address;
    ENetHost *server;

    address.host = ENET_HOST_ANY;
    address.port = 60000;
    server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server)
    {
        std::cerr << "[SERVER] Failed to create ENet server.\n";
        return;
    }

    std::cout << "[SERVER] Listening on port 60000...\n";

    ENetEvent event;
    std::vector<char> buffer(MESSAGE_SIZE, 'A');

    while (true)
    {
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "[SERVER] Client connected.\n";
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "[SERVER] Received message from client.\n";
                enet_peer_send(event.peer, 0, enet_packet_create(buffer.data(), buffer.size(), ENET_PACKET_FLAG_RELIABLE));
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "[SERVER] Client disconnected.\n";
                break;

            default:
                break;
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
}

void client()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "[CLIENT] Failed to initialize ENet.\n";
        return;
    }

    ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);

    if (!client)
    {
        std::cerr << "[CLIENT] Failed to create ENet client.\n";
        return;
    }

    ENetAddress address;
    ENetPeer *peer;

    enet_address_set_host(&address, "localhost");
    address.port = 60000;

    peer = enet_host_connect(client, &address, 2, 0);

    if (!peer)
    {
        std::cerr << "[CLIENT] Failed to connect to server.\n";
        return;
    }

    ENetEvent event;
    std::vector<char> message(MESSAGE_SIZE, 'A');
    std::vector<double> throughput_values;
    int test_count = 0;

    while (enet_host_service(client, &event, 5000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            std::cout << "[CLIENT] Connected to server.\n";
            break;
        }
        else
        {
            std::cerr << "[CLIENT] Connection to server failed.\n";
            return;
        }
    }

    while (test_count < TEST_AMOUNT)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_MESSAGES; ++i)
        {
            ENetPacket *packet = enet_packet_create(message.data(), message.size(), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);

            while (enet_host_service(client, &event, 5000) > 0)
            {
                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                {
                    enet_packet_destroy(event.packet);
                    break;
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        double total_data_transferred = static_cast<double>(NUM_MESSAGES * MESSAGE_SIZE) / (1024.0 * 1024.0); // MB
        double throughput = total_data_transferred / duration.count();
        throughput_values.push_back(throughput);

        std::cout << "[CLIENT] Test " << (test_count + 1) << ": Throughput = " << throughput << " MB/s\n";
        ++test_count;
    }

    std::cout << "[CLIENT] All throughput values:\n";
    for (const auto &value : throughput_values)
    {
        std::cout << value << " MB/s\n";
    }

    enet_peer_disconnect(peer, 0);

    while (enet_host_service(client, &event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << "[CLIENT] Disconnected from server.\n";
            return;
        }
    }

    enet_host_destroy(client);
    enet_deinitialize();
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    if (std::string(argv[1]) == "--server")
    {
        server();
    }
    else if (std::string(argv[1]) == "--client")
    {
        client();
    }
    else
    {
        std::cerr << "Invalid mode. Use --server or --client\n";
        return 1;
    }

    return 0;
}
