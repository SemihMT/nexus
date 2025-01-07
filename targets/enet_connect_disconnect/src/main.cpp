#include <enet/enet.h>
#include <iostream>
#include <string>

enum class MyMessageType : uint32_t
{
    Connect,
    Disconnect,
};

void run_server() {
    if (enet_initialize() != 0) {
        std::cerr << "[SERVER] Failed to initialize ENet.\n";
        return;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    ENetHost* server;

    address.host = ENET_HOST_ANY;
    address.port = 60000;

    server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) {
        std::cerr << "[SERVER] Failed to create ENet server.\n";
        return;
    }

    std::cout << "[SERVER] Server started on port " << address.port << "\n";

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    char client_ip[16];
                    enet_address_get_host_ip(&event.peer->address, client_ip, sizeof(client_ip));

                    std::cout << "[SERVER] Client connected from "
                              << client_ip << ":" << event.peer->address.port << "\n";

                    // Disconnect the client immediately
                    enet_peer_disconnect(event.peer, 0);
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
}

void run_client() {
    if (enet_initialize() != 0) {
        std::cerr << "[CLIENT] Failed to initialize ENet.\n";
        return;
    }
    atexit(enet_deinitialize);

    ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);
    if (!client) {
        std::cerr << "[CLIENT] Failed to create ENet client.\n";
        return;
    }

    ENetAddress address;
    ENetPeer* peer;

    enet_address_set_host(&address, "localhost");
    address.port = 60000;

    peer = enet_host_connect(client, &address, 2, 0);
    if (!peer) {
        std::cerr << "[CLIENT] Failed to create ENet connection.\n";
        enet_host_destroy(client);
        return;
    }

    std::cout << "[CLIENT] Connecting to server...\n";

    ENetEvent event;
    bool running = true;

    while (running) {
        while (enet_host_service(client, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "[CLIENT] Connected to server.\n";
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "[CLIENT] Disconnected from server.\n";
                    running = false;
                    break;

                default:
                    break;
            }
        }
    }

    enet_host_destroy(client);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    if (std::string(argv[1]) == "--server") {
        run_server();
    } else if (std::string(argv[1]) == "--client") {
        run_client();
    } else {
        std::cerr << "Invalid mode. Use --server or --client\n";
        return 1;
    }

    return 0;
}
