#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include "connection.h"
#include "asio.hpp"
#include <functional>
#include <optional>
#include <unordered_map>
#include <memory>
#include <future>

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
        explicit Server(uint16_t port = 60000)
            : m_Port{port}, m_IOContext{Nexus::GetInstance().GetContext()}, m_Strand{asio::make_strand(m_IOContext)}, m_Acceptor{m_IOContext, tcp::endpoint{tcp::v4(), m_Port}}
        {
        }

        // -----------------------------------------------------------------------
        // Handler registration
        // Must be called before Run() — no concurrent access at this point.
        // -----------------------------------------------------------------------
        void AddMessageHandler(
            MessageType messageType,
            std::function<void(std::shared_ptr<Connection<MessageType>>,
                               Message<MessageType> &)>
                handler)
        {
            m_MessageHandlers[messageType] = std::move(handler);
        }

        void AddEventHandler(
            ServerEvent event,
            std::function<void(std::shared_ptr<Connection<MessageType>>)> handler)
        {
            m_EventHandlers[event] = std::move(handler);
        }

        // -----------------------------------------------------------------------
        // Called by Connection (from a connection strand thread).
        // Post onto the server strand so m_MessageHandlers is accessed safely.
        // -----------------------------------------------------------------------
        void ProcessIncomingMessage(
            std::shared_ptr<Connection<MessageType>> connection,
            Message<MessageType> message) // by value — safe to move into lambda
        {
            asio::post(m_Strand,
                       [this, connection, message = std::move(message)]() mutable
                       {
                           auto it = m_MessageHandlers.find(message.m_Header.type);
                           if (it != m_MessageHandlers.end())
                               it->second(connection, message);
                           else
                               std::cerr << "No handler for message type "
                                         << static_cast<uint32_t>(message.m_Header.type) << "\n";
                       });
        }

        // -----------------------------------------------------------------------
        // Run — kicks off the accept loop.
        // Block = true: parks the calling thread cleanly (no spin).
        // -----------------------------------------------------------------------
        void Run(bool shouldBlock = false)
        {
            StartAccept();

            if (shouldBlock)
            {
                // Park here until Shutdown() signals us.
                m_ShutdownFuture = m_ShutdownPromise.get_future();
                m_ShutdownFuture.wait();
            }
        }

        // -----------------------------------------------------------------------
        // Shutdown — safe to call from any thread.
        // -----------------------------------------------------------------------
        void Shutdown()
        {
            asio::post(m_Strand, [this]()
                       {
                           // Disconnect all connections (each Disconnect() posts to that
                           // connection's own strand, so it's safe to call here).
                           for (auto &[id, conn] : m_Connections)
                               conn->Disconnect();

                           m_Connections.clear();
                           m_Acceptor.close();

                           // Unblock Run(true) if it's waiting.
                           try
                           {
                               m_ShutdownPromise.set_value();
                           }
                           catch (const std::future_error &)
                           {
                           } // already set — ignore
                       });
        }

        // -----------------------------------------------------------------------
        // SendToClient — safe to call from any thread (e.g. game loop).
        // -----------------------------------------------------------------------
        void SendToClient(uint32_t clientID, const Message<MessageType> &message)
        {
            asio::post(m_Strand,
                       [this, clientID, message]()
                       {
                           auto it = m_Connections.find(clientID);
                           if (it != m_Connections.end())
                               it->second->Send(message);
                           else
                               std::cerr << "Client ID " << clientID << " not found.\n";
                       });
        }

        // -----------------------------------------------------------------------
        // Broadcast — safe to call from any thread.
        // -----------------------------------------------------------------------
        void Broadcast(const Message<MessageType> &message,
                       std::optional<uint32_t> IDToSkip = std::nullopt)
        {
            asio::post(m_Strand,
                       [this, message, IDToSkip]()
                       {
                           for (auto &[id, conn] : m_Connections)
                           {
                               if (IDToSkip.has_value() && id == IDToSkip.value())
                                   continue;
                               conn->Send(message);
                           }
                       });
        }

        // -----------------------------------------------------------------------
        // Called by Connection::DoDisconnect() (on that connection's strand).
        // Post onto the server strand so m_Connections is accessed safely.
        // -----------------------------------------------------------------------
        void OnConnectionDisconnected(
            std::shared_ptr<Connection<MessageType>> connection)
        {
            asio::post(m_Strand,
                       [this, connection]()
                       {
                           CallEventHandler(ServerEvent::OnDisconnect, connection);
                           m_Connections.erase(connection->GetID());
                       });
        }

        // -----------------------------------------------------------------------
        // GetServerAddress — synchronous, safe to call before/after Run().
        // -----------------------------------------------------------------------
        std::string GetServerAddress()
        {
            auto endpoint = m_Acceptor.local_endpoint();

            asio::ip::tcp::resolver resolver(m_IOContext);
            auto results = resolver.resolve(asio::ip::host_name(), "");

            for (const auto &entry : results)
            {
                auto addr = entry.endpoint().address();
                if (addr.is_v4() && !addr.is_loopback())
                    return addr.to_string() + ":" + std::to_string(endpoint.port());
            }

            return "127.0.0.1:" + std::to_string(endpoint.port());
        }

    private:
        // -----------------------------------------------------------------------
        // Accept loop — runs on the server strand.
        // -----------------------------------------------------------------------
        void StartAccept()
        {
            auto connectionPtr = Connection<MessageType>::Create();
            connectionPtr->SetServer(this);

            if (!m_Acceptor.is_open())
                return;

            m_Acceptor.async_accept(
                connectionPtr->socket(),
                asio::bind_executor(m_Strand,
                                    std::bind(&Server::HandleAccept, this,
                                              connectionPtr,
                                              asio::placeholders::error)));
        }

        void HandleAccept(std::shared_ptr<Connection<MessageType>> newConnection,
                          const asio::error_code &error)
        {
            // Always queue the next accept first so we don't stall the accept
            // loop while the handshake for this connection is in flight.
            StartAccept();

            if (error)
            {
                if (error != asio::error::operation_aborted)
                    std::cerr << "Accept error: " << error.message() << "\n";
                return;
            }

            uint32_t newClientID = m_NextID++;
            newConnection->SetID(newClientID);
            m_Connections[newClientID] = newConnection;

            // Send the assigned client ID. Bind through the connection's own
            // strand — this write races with nothing because the connection
            // hasn't called Start() yet and the socket is ours alone here.
            asio::async_write(
                newConnection->socket(),
                asio::buffer(&newClientID, sizeof(newClientID)),
                asio::bind_executor(newConnection->strand(),
                                    [this, newConnection](const asio::error_code &ec, std::size_t)
                                    {
                                        if (!ec)
                                        {
                                            newConnection->Start();
                                            // Post the user callback back onto the server strand
                                            // so it can safely call Broadcast/SendToClient etc.
                                            asio::post(m_Strand, [this, newConnection]()
                                                       { CallEventHandler(ServerEvent::OnConnect, newConnection); });
                                        }
                                        else
                                        {
                                            std::cerr << "Error sending client ID: " << ec.message() << "\n";
                                        }
                                    }));
        }

        void CallEventHandler(ServerEvent event,
                              std::shared_ptr<Connection<MessageType>> connection)
        {
            auto it = m_EventHandlers.find(event);
            if (it != m_EventHandlers.end())
                it->second(connection);
        }

    private:
        uint16_t m_Port{60000};
        asio::io_context &m_IOContext;

        // All access to m_Connections, m_NextID, and the handler maps is
        // serialised through this strand. No mutex needed.
        asio::strand<asio::io_context::executor_type> m_Strand;

        tcp::acceptor m_Acceptor;

        std::unordered_map<MessageType,
                           std::function<void(std::shared_ptr<Connection<MessageType>>,
                                              Message<MessageType> &)>>
            m_MessageHandlers;

        std::unordered_map<ServerEvent,
                           std::function<void(std::shared_ptr<Connection<MessageType>>)>>
            m_EventHandlers;

        uint32_t m_NextID{1001};
        std::unordered_map<uint32_t,
                           std::shared_ptr<Connection<MessageType>>>
            m_Connections;

        // Used by Run(true) / Shutdown() to park and unpark the calling thread.
        std::promise<void> m_ShutdownPromise;
        std::future<void> m_ShutdownFuture;
    };

} // namespace nxs

#endif // SERVER_H
