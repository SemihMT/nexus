#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include "connection.h"
#include "asio.hpp"
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <unordered_map>

namespace nxs
{
    template <typename MessageType>
    class Server
    {
        // Handler type aliases — used internally and kept private so users
        // never need to spell them out.
        using ConnectionPtr = std::shared_ptr<Connection<MessageType>>;
        using EventHandler = std::function<void(ConnectionPtr)>;
        using MessageHandler = std::function<void(ConnectionPtr, Message<MessageType> &)>;

    public:
        // -----------------------------------------------------------------------
        // Construction
        // -----------------------------------------------------------------------
        explicit Server(uint16_t port = 60000)
            : m_Port{port}, m_IOContext{Nexus::GetInstance().GetContext()}, m_Strand{asio::make_strand(m_IOContext)}, m_Acceptor{m_IOContext, tcp::endpoint{tcp::v4(), m_Port}}
        {
        }

        // -----------------------------------------------------------------------
        // Fluent API — call before Run(), returns *this for chaining.
        // -----------------------------------------------------------------------

        /// Called when a client connects. Provides the Connection.
        Server &OnConnect(EventHandler handler)
        {
            m_EventHandlers[Event::Connect] = std::move(handler);
            return *this;
        }

        /// Called when a client disconnects. Provides the Connection.
        Server &OnDisconnect(EventHandler handler)
        {
            m_EventHandlers[Event::Disconnect] = std::move(handler);
            return *this;
        }

        /// Called when a message of the given type arrives from a client.
        Server &OnMessage(MessageType type, MessageHandler handler)
        {
            m_MessageHandlers[type] = std::move(handler);
            return *this;
        }

        // -----------------------------------------------------------------------
        // Run — starts the accept loop and returns immediately.
        // Use this when you have your own game/render loop.
        // -----------------------------------------------------------------------
        Server &Run()
        {
            StartAccept();
            return *this;
        }

        // -----------------------------------------------------------------------
        // RunBlocking — starts the accept loop and parks the calling thread
        // until Shutdown() is called. Use this for headless servers with no
        // external loop.
        // -----------------------------------------------------------------------
        void RunBlocking()
        {
            StartAccept();
            m_ShutdownFuture = m_ShutdownPromise.get_future();
            m_ShutdownFuture.wait();
        }

        // -----------------------------------------------------------------------
        // Shutdown — safe to call from any thread.
        // -----------------------------------------------------------------------
        void Shutdown()
        {
            asio::post(m_Strand, [this]()
                       {
                for (auto &[id, conn] : m_Connections)
                    conn->Disconnect();

                m_Connections.clear();
                m_Acceptor.close();

                try { m_ShutdownPromise.set_value(); }
                catch (const std::future_error &) {} });
        }

        // -----------------------------------------------------------------------
        // Messaging — safe to call from any thread (e.g. game loop).
        // -----------------------------------------------------------------------
        void SendToClient(uint32_t clientID, const Message<MessageType> &message)
        {
            asio::post(m_Strand, [this, clientID, message]()
                       {
                auto it = m_Connections.find(clientID);
                if (it != m_Connections.end())
                    it->second->Send(message);
                else
                    std::cerr << "SendToClient: ID " << clientID << " not found.\n"; });
        }

        void Broadcast(const Message<MessageType> &message,
                       std::optional<uint32_t> skipID = std::nullopt)
        {
            asio::post(m_Strand, [this, message, skipID]()
                       {
                for (auto &[id, conn] : m_Connections)
                {
                    if (skipID.has_value() && id == skipID.value()) continue;
                    conn->Send(message);
                } });
        }

        // -----------------------------------------------------------------------
        // Utility
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

        // -----------------------------------------------------------------------
        // Called by Connection internals — not intended for user code.
        // -----------------------------------------------------------------------
        void ProcessIncomingMessage(ConnectionPtr connection, Message<MessageType> message)
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

        void OnConnectionDisconnected(ConnectionPtr connection)
        {
            asio::post(m_Strand, [this, connection]()
                       {
                FireEvent(Event::Disconnect, connection);
                m_Connections.erase(connection->GetID()); });
        }

    private:
        enum class Event
        {
            Connect,
            Disconnect
        };

        void StartAccept()
        {
            auto conn = Connection<MessageType>::Create();
            conn->SetServer(this);

            if (!m_Acceptor.is_open())
                return;

            m_Acceptor.async_accept(
                conn->socket(),
                asio::bind_executor(m_Strand,
                                    std::bind(&Server::HandleAccept, this, conn,
                                              asio::placeholders::error)));
        }

        void HandleAccept(ConnectionPtr newConn, const asio::error_code &error)
        {
            StartAccept(); // re-arm immediately

            if (error)
            {
                if (error != asio::error::operation_aborted)
                    std::cerr << "Accept error: " << error.message() << "\n";
                return;
            }

            uint32_t id = m_NextID++;
            newConn->SetID(id);
            m_Connections[id] = newConn;

            asio::async_write(
                newConn->socket(),
                asio::buffer(&id, sizeof(id)),
                asio::bind_executor(newConn->strand(),
                                    [this, newConn](const asio::error_code &ec, std::size_t)
                                    {
                                        if (!ec)
                                        {
                                            newConn->Start();
                                            asio::post(m_Strand, [this, newConn]()
                                                       { FireEvent(Event::Connect, newConn); });
                                        }
                                        else
                                        {
                                            std::cerr << "Error sending client ID: " << ec.message() << "\n";
                                        }
                                    }));
        }

        void FireEvent(Event event, ConnectionPtr connection)
        {
            auto it = m_EventHandlers.find(event);
            if (it != m_EventHandlers.end())
                it->second(connection);
        }

    private:
        uint16_t m_Port{60000};
        asio::io_context &m_IOContext;
        asio::strand<asio::io_context::executor_type> m_Strand;
        tcp::acceptor m_Acceptor;

        std::unordered_map<MessageType, MessageHandler> m_MessageHandlers;
        std::unordered_map<Event, EventHandler> m_EventHandlers;

        uint32_t m_NextID{1001};
        std::unordered_map<uint32_t, ConnectionPtr> m_Connections;

        std::promise<void> m_ShutdownPromise;
        std::future<void> m_ShutdownFuture;
    };

} // namespace nxs

#endif // SERVER_H
