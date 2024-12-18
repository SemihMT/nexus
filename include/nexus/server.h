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
        Server(uint16_t port = 60000) : m_Port{port}, m_IOContext{Nexus::GetInstance().GetContext()}, m_Acceptor{m_IOContext,tcp::endpoint{tcp::v4(),m_Port}}
        {
            StartAccept();
        }
        private:
        /**
         * @brief Creates a Connection pointer using Connection::Create(), listens and accepts any connection request on Connection::m_socket
         * 
         * 
         */
        void StartAccept()
        {
            auto connectionPtr = Connection::Create();
            m_Acceptor.async_accept(connectionPtr->socket(),std::bind(&Server::HandleAccept, this, connectionPtr, asio::placeholders::error));
        }
        void HandleAccept(std::shared_ptr<Connection> newConnection, const std::error_code& error)
        {
            if(!error)
            {
                newConnection->Start();
            }
            StartAccept();
        }
        private:
        /**
         * @brief The port number for this server
         */
        uint16_t m_Port{60000};

        /**
         * @brief The ASIO IO Context
         */
        asio::io_context& m_IOContext;

        /**
         * @brief The ASIO TCP Acceptor object, handles accepting incoming connections on the port 
         */
        tcp::acceptor m_Acceptor;
    };
}
#endif