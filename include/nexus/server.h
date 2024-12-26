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

    class Server
    {
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

        void AddEventHandler(const std::string &eventName, std::function<void()> handler)
        {
            m_EventHandlers[eventName] = handler;
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
        void Send(std::shared_ptr<Connection> connection, const std::string &message)
        {
            if(connection)
                connection->Send(message);
        }
        void Send(const std::string &message)
        {
            for (auto &connection : m_Connections)
            {
                connection->Send(message);
            }
        }
        void OnConnectionDisconnected(std::shared_ptr<Connection> connection)
        {
            CallEventHandler("OnDisconnect");
            m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), connection), m_Connections.end());
        }
    private:
        /**
         * @brief Creates a Connection pointer using Connection::Create(),
         * listens and accepts any connection request on Connection::m_socket
         */
        void StartAccept()
        {
            auto connectionPtr = Connection::Create();
            connectionPtr->SetServer(this);
            m_Acceptor.async_accept(connectionPtr->socket(), std::bind(&Server::HandleAccept, this, connectionPtr, asio::placeholders::error));
        }
        void HandleAccept(std::shared_ptr<Connection> newConnection, const std::error_code &error)
        {
            if (!error)
            {
                CallEventHandler("OnConnect");
                newConnection->Start();
                m_Connections.push_back(newConnection);
            }
            StartAccept();
        }

        void CallEventHandler(const std::string &eventName)
        {
            if (m_EventHandlers.find(eventName) != m_EventHandlers.end())
            {
                m_EventHandlers[eventName]();
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
        //map of event handler functions
        std::unordered_map<std::string, std::function<void()>> m_EventHandlers;
        std::vector<std::shared_ptr<Connection>> m_Connections;
    };
}
#endif