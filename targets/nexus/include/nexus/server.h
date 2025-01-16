#ifndef SERVER_H
#define SERVER_H
#include "connection.h"
#include "asio.hpp"
// Windows version definition (versions of windows handle networking differently?)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif
/**
 * @brief The Server class
 *
 */
namespace nxs
{
    template <typename MessageType>
    class Server
    {
    public:
        enum class ServerEvent
        {
            OnConnect,
            OnDisconnect
        };

    public:
        /**
         * @brief Construct a new Server object
         *
         * @param port The TCP port number this server should listen on
         */
        Server(uint16_t port = 60000) : m_Port{port},
                                        m_IOContext{Nexus::GetInstance().GetContext()},
                                        m_Acceptor{m_IOContext, tcp::endpoint{tcp::v4(), m_Port}}
        {
        }

        void AddMessageHandler(MessageType messageType, std::function<void(std::shared_ptr<Connection<MessageType>>, Message<MessageType> &)> handler)
        {
            m_MessageHandlers[messageType] = handler;
        }
        void AddEventHandler(ServerEvent event, std::function<void(std::shared_ptr<Connection<MessageType>>)> handler)
        {
            m_EventHandlers[event] = handler;
        }
        void ProcessIncomingMessage(std::shared_ptr<Connection<MessageType>> connection, Message<MessageType> &message)
        {
            auto it = m_MessageHandlers.find(message.m_Header.type);
            if (it != m_MessageHandlers.end())
            {
                it->second(connection, message);
            }
            else
            {
                std::cerr << "No handler registered for message type: "
                          << static_cast<uint32_t>(message.m_Header.type) << "\n";
            }
        }

        void Run(bool shouldBlock = false)
        {
            StartAccept();
            // If there's no game loop implemented, it's up to the user to block the process so the server can run
            if (shouldBlock)
            {
                while (true)
                {
                }
            }
        }
        void Shutdown()
        {
            // Close all connections
            for (auto &[id, connection] : m_Connections)
            {
                connection->Disconnect();
            }
            // Close the acceptor
            m_Acceptor.close();
        }
        // Send a message to a specific client by ID
        void SendToClient(uint32_t clientID, const Message<MessageType> &message)
        {
            auto it = m_Connections.find(clientID);
            if (it != m_Connections.end())
            {
                it->second->Send(message);
            }
            else
            {
                std::cerr << "Client ID " << clientID << " not found.\n";
            }
        }

        // Broadcast a message to all clients
        void Broadcast(const Message<MessageType> &message, std::optional<uint32_t> IDToSkip = std::nullopt)
        {
            for (auto &[id, connection] : m_Connections)
            {
                if (IDToSkip.has_value() && id == IDToSkip.value())
                {
                    continue; // Skip the specified client ID
                }

                connection->Send(message);
            }
        }
        void OnConnectionDisconnected(std::shared_ptr<Connection<MessageType>> connection)
        {
            CallEventHandler(ServerEvent::OnDisconnect, connection);
            // Remove connection from map
            m_Connections.erase(connection->GetID());
        }
        std::string GetServerAddress()
        {
            // Get the local endpoint (0.0.0.0:port)
            auto endpoint = m_Acceptor.local_endpoint();

            // Resolve the machine's hostname to get its IP addresses
            asio::ip::tcp::resolver resolver(m_IOContext);
            auto results = resolver.resolve(asio::ip::host_name(), "");

            // Use the first non-loopback IPv4 address
            for (const auto &entry : results)
            {
                auto addr = entry.endpoint().address();
                if (addr.is_v4() && !addr.is_loopback())
                {
                    // Return the actual IP address and port
                    return addr.to_string() + ":" + std::to_string(endpoint.port());
                }
            }

            // Fallback: If no non-loopback IPv4 address is found, return the loopback address
            return "127.0.0.1:" + std::to_string(endpoint.port());
        }

    private:
        /**
         * @brief Creates a Connection pointer using Connection::Create(),
         * listens and accepts any connection request on Connection::m_socket
         */
        void StartAccept()
        {
            auto connectionPtr = Connection<MessageType>::Create();
            connectionPtr->SetServer(this);
            if (m_Acceptor.is_open())
                m_Acceptor.async_accept(connectionPtr->socket(), std::bind(&Server::HandleAccept, this, connectionPtr, asio::placeholders::error));
        }
        void HandleAccept(std::shared_ptr<Connection<MessageType>> newConnection, const std::error_code &error)
        {
            if (!error)
            {
                uint32_t newClientID = m_NextID++;
                newConnection->SetID(newClientID);
                m_Connections[newClientID] = newConnection;
                // Send the client ID immediately
                asio::async_write(
                    newConnection->socket(),
                    asio::buffer(&newClientID, sizeof(newClientID)),
                    [this, newConnection](const asio::error_code &error, std::size_t /*length*/)
                    {
                        if (!error)
                        {
                            newConnection->Start(); // Start the connection normally
                            CallEventHandler(ServerEvent::OnConnect, newConnection);
                        }
                        else
                        {
                            std::cerr << "Error sending client ID: " << error.message() << "\n";
                        }
                    });
            }
            StartAccept();
        }

        void CallMessageHandler(MessageType message)
        {
            if (m_MessageHandlers.find(message) != m_MessageHandlers.end())
            {
                m_MessageHandlers[message]();
            }
        }
        void CallEventHandler(ServerEvent event, std::shared_ptr<Connection<MessageType>> connection)
        {
            if (m_EventHandlers.find(event) != m_EventHandlers.end())
            {
                m_EventHandlers[event](connection);
            }
        }

    private:
        /**
         * @brief The port number for this server
         */
        uint16_t m_Port{60000};

        /**
         * @brief The ASIO IO Context
         */
        asio::io_context &m_IOContext;

        /**
         * @brief The ASIO TCP Acceptor object, handles accepting incoming connections on the port
         */
        tcp::acceptor m_Acceptor;

        /// Map of event handlers
        std::unordered_map<MessageType, std::function<void(std::shared_ptr<Connection<MessageType>>, Message<MessageType> &)>> m_MessageHandlers;
        /// Map of event handlers
        std::unordered_map<ServerEvent, std::function<void(std::shared_ptr<Connection<MessageType>>)>> m_EventHandlers;

        /// The next ID to be assigned to a connection
        uint32_t m_NextID{1001};
        /// Map of IDs to connections
        std::unordered_map<uint32_t, std::shared_ptr<Connection<MessageType>>> m_Connections;
    };
}; // namespace nxs
#endif