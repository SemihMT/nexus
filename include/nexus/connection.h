#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <memory>
#include <asio.hpp>
#include <array>
#include "nexus.h"

namespace nxs
{
    using asio::ip::tcp;
    class Server;

    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        static std::shared_ptr<Connection> Create();

        tcp::socket &socket();
        void Start();
        void Send(const std::string &message);

        void SetServer(nxs::Server* server);
        std::string m_message;
    private:
        Connection(asio::io_context &ctx);

        void StartRead();

        void HandleRead(const std::error_code &error, std::size_t bytesTransferred);

        void StartWrite();

        void HandleWrite(const std::error_code &error, size_t bytesTransferred);

       

    private:
        tcp::socket m_socket;
        std::array<char,1024> m_buffer; // Buffer for reading incoming messages
        nxs::Server* m_server;
    };
}

#endif
