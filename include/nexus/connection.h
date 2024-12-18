#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <memory>
#include "nexus.h"

namespace nxs
{
    using asio::ip::tcp;
    /**
     * @brief The Connection class simplifies the operations we can perform on an established connection between the server and client
     *
     */
    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        /**
         * @brief Creates a connection object, disabled ctor in favor of this to ensure this object is managed by a shared_ptr
         *
         * @return std::shared_ptr<Connection>
         */
        static std::shared_ptr<Connection> Create()
        {
            auto sharedConnectionPtr = std::shared_ptr<Connection>(new Connection(Nexus::GetInstance().GetContext()));
            return sharedConnectionPtr;
        }
        /**
         * @brief asio::tcp::socket getter
         *
         * @return tcp::socket&
         */
        tcp::socket &socket()
        {
            return m_socket;
        }
        /**
         * @brief WIP: Currently sends the string "Hello Socket!" over the socket.
         *
         */
        void Start()
        {
            // Send a predefined message for now
            // Modify this so the user can define what they want to send and when
            asio::async_write(
                m_socket,
                asio::buffer("Hello Socket!"),
                std::bind(&Connection::HandleWrite, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
        }

    private:
        /**
         * @brief Private constructor for the Connection object to prevent one being created accidentally
         *
         * @param ctx The IO Context
         */
        Connection(asio::io_context &ctx) : m_socket{ctx}
        {
        }
        /**
         * @brief Handler function that gets called after Connection::Start() has finished/failed sending data over the socket
         *
         */
        void HandleWrite(const std::error_code & /*error*/, size_t bytesTransferred)
        {
            // This function is called after the async_write is done writing to the socket
            std::cout << "Number of bytes transferred: [" << bytesTransferred << "]" << "\n";
        }

    private:
        tcp::socket m_socket;
    };
}
#endif