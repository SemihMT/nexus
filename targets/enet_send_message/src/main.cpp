#include <enet/enet.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>

// Message Types
enum class MyMessageType : uint32_t
{
    ChatMessage
};

// A simple packet structure
struct ChatPacket
{
    MyMessageType type;
    uint32_t senderID;    // The sender's client ID
    uint32_t recipientID; // The recipient's client ID
    char message[256];    // Message content
};

void RunServer()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to initialize ENet.\n";
        return;
    }

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = 60000;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0); // Allow up to 32 connections
    if (!server)
    {
        std::cerr << "Failed to create ENet server.\n";
        return;
    }

    std::cout << "[SERVER] Server is running on port 60000...\n";

    std::unordered_map<uint32_t, ENetPeer*> clientMap; // Maps client ID to peer
    uint32_t nextClientID = 1001;

    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    uint32_t clientID = nextClientID++;
                    event.peer->data = reinterpret_cast<void*>(clientID);
                    clientMap[clientID] = event.peer;
                    std::cout << "[SERVER] Client [" << clientID << "] connected.\n";
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    ChatPacket packet;
                    if (event.packet->dataLength != sizeof(ChatPacket))
                    {
                        std::cerr << "[SERVER] Received packet with invalid size. Disconnecting client.\n";
                        enet_peer_disconnect(event.peer, 0);
                        break;
                    }

                    std::memcpy(&packet, event.packet->data, sizeof(ChatPacket));

                    if (packet.type == MyMessageType::ChatMessage)
                    {
                        uint32_t senderID = packet.senderID;
                        std::cout << "[SERVER] Message from client [" << senderID << "] to client [" << packet.recipientID << "]: " << packet.message << "\n";

                        // Forward the message to the recipient
                        if (clientMap.find(packet.recipientID) != clientMap.end())
                        {
                            ENetPeer* recipientPeer = clientMap[packet.recipientID];
                            ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(ChatPacket), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(recipientPeer, 0, enetPacket); // Forward the packet
                        }
                        else
                        {
                            std::cout << "[SERVER] Recipient [" << packet.recipientID << "] not found.\n";
                        }
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    uint32_t clientID = reinterpret_cast<uintptr_t>(event.peer->data);
                    std::cout << "[SERVER] Client [" << clientID << "] disconnected.\n";
                    clientMap.erase(clientID);
                    event.peer->data = nullptr;
                    break;
                }
                default:
                    break;
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
}

void RunClient(uint32_t clientID)
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to initialize ENet.\n";
        return;
    }

    ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client)
    {
        std::cerr << "Failed to create ENet client.\n";
        return;
    }

    ENetAddress address{};
    enet_address_set_host(&address, "localhost");
    address.port = 60000;

    ENetPeer* serverPeer = enet_host_connect(client, &address, 2, 0);
    if (!serverPeer)
    {
        std::cerr << "Failed to connect to the server.\n";
        return;
    }

    std::cout << "[CLIENT " << clientID << "] Connecting to server...\n";

    bool isConnected = false;

    while (true)
    {
        ENetEvent event;
        while (enet_host_service(client, &event, 1000) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    std::cout << "[CLIENT " << clientID << "] Connected to server.\n";
                    isConnected = true;

                    // Send a chat message if the client ID is 1002
                    if (clientID == 1002)
                    {
                        ChatPacket packet{};
                        packet.type = MyMessageType::ChatMessage;
                        packet.senderID = clientID;
                        packet.recipientID = 1001; // Send to client 1001
                        std::strncpy(packet.message, "Hello, client 1001!", sizeof(packet.message) - 1);

                        ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(ChatPacket), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(serverPeer, 0, enetPacket);
                    }
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    ChatPacket packet;
                    if (event.packet->dataLength != sizeof(ChatPacket))
                    {
                        std::cerr << "[CLIENT " << clientID << "] Received packet with invalid size.\n";
                        break;
                    }

                    std::memcpy(&packet, event.packet->data, sizeof(ChatPacket));

                    if (packet.type == MyMessageType::ChatMessage)
                    {
                        std::cout << "[CLIENT " << clientID << "] Message from client [" << packet.senderID << "]: " << packet.message << "\n";
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    std::cout << "[CLIENT " << clientID << "] Disconnected from server.\n";
                    isConnected = false;
                    return;
                }
                default:
                    break;
            }
        }

        if (!isConnected)
        {
            break;
        }
    }

    enet_host_destroy(client);
    enet_deinitialize();
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client <clientID>\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "--server")
    {
        RunServer();
    }
    else if (mode == "--client")
    {
        uint32_t clientID = std::stoi(argv[2]);
        RunClient(clientID);
    }
    else
    {
        std::cerr << "Invalid mode. Use --server or --client <clientID>\n";
        return 1;
    }

    return 0;
}
