#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <memory>
#include <asio.hpp>
#include <array>
#include "nexus.h"
#include "message.h"

namespace nxs
{
    using asio::ip::tcp;
    template <typename MessageType>
    class Server;
    template <typename MessageType>
    class Connection : public std::enable_shared_from_this<Connection<MessageType>>
    {
    public:
        static std::shared_ptr<Connection<MessageType>> Create()
        {
            auto sharedConnectionPtr = std::shared_ptr<Connection<MessageType>>(new Connection<MessageType>(Nexus::GetInstance().GetContext()));
            return sharedConnectionPtr;
        }

        tcp::socket &socket()
        {
            return m_socket;
        }
        void Start()
        {
            // Start listening for incoming messages
            StartRead();
        }
        void Send(const Message<MessageType> &message)
        {
            // If the queue is empty: we're not currently writing
            // If the queue is not empty: we're currently writing
            bool writeInProgress = !m_OutgoingMessages.empty();
            m_OutgoingMessages.push(message);
            if (!writeInProgress)
            {
                StartWrite();
            }
        }

        void SetServer(nxs::Server<MessageType> *server)
        {
            m_server = server;
        }
        void SetID(uint32_t id)
        {
            m_ID = id;
        }
        uint32_t GetID() const
        {
            return m_ID;
        }

    private:
        Connection(asio::io_context &ctx) : m_socket{ctx},m_TemporaryMessage{static_cast<MessageType>(std::numeric_limits<uint32_t>::max())} {}

        void StartRead()
        {
            // Start reading the header first
            asio::async_read(
                m_socket,
                asio::buffer(&m_TemporaryHeader, sizeof(Header<MessageType>)),
                std::bind(
                    &Connection::HandleReadHeader,
                    this->shared_from_this(),
                    asio::placeholders::error));
        }
        void HandleReadHeader(const asio::error_code &error)
        {
            if (!error)
            {
                // Resize the temporary message body based on the size from the header
                m_TemporaryMessage.m_Header = m_TemporaryHeader;
                m_TemporaryMessage.m_Data.resize(m_TemporaryHeader.size);

                if (m_TemporaryHeader.size > 0)
                {
                    // Read the message body
                    asio::async_read(
                        m_socket,
                        asio::buffer(m_TemporaryMessage.m_Data.data(), m_TemporaryHeader.size),
                        std::bind(
                            &Connection::HandleReadBody,
                            this->shared_from_this(),
                            asio::placeholders::error));
                }
                else
                {
                    // No body, process the message immediately
                    OnMessageReceived();
                    StartRead();
                }
            }
            else
            {
                HandleReadError(error);
            }
        }
        void HandleReadBody(const asio::error_code &error)
        {
            if (!error)
            {
                // Entire message has been received
                OnMessageReceived();
                StartRead(); // Continue reading the next message
            }
            else
            {
                HandleReadError(error);
            }
        }
        // void HandleRead(const std::error_code &error, std::size_t bytesTransferred)
        // {
        //     if (!error)
        //     {
        //         // Process the received data
        //         std::string message(m_buffer.data(), bytesTransferred);
        //         std::cout << "Message from client: " << message << "\n";

        //         // Continue reading
        //         StartRead();
        //     }
        //     else if (error == asio::error::eof)
        //     {
        //         m_server->OnConnectionDisconnected(this->shared_from_this());
        //     }
        //     else if (error == asio::error::operation_aborted)
        //     {
        //         m_server->OnConnectionDisconnected(this->shared_from_this());
        //     }
        //     else
        //     {
        //         std::cerr << "Error reading from socket: " << error.message() << "\n";
        //     }
        // }
        void OnMessageReceived()
        {
            // Here, you can process the complete message
            //std::cout << "Received message: " << m_TemporaryMessage << "\n";
            // Add to incoming queue or directly process
            m_server->ProcessIncomingMessage(this->shared_from_this(), m_TemporaryMessage);
        }

        void HandleReadError(const std::error_code &error)
        {
            if (error == asio::error::eof)
            {
                m_server->OnConnectionDisconnected(this->shared_from_this());
            }
            else if (error == asio::error::operation_aborted)
            {
                m_server->OnConnectionDisconnected(this->shared_from_this());
            }
            else
            {
                std::cerr << "Error reading from socket: " << error.message() << "\n";
            }
        }
        void StartWrite()
        {
            // Get the message at the front of the queue
            auto &message = m_OutgoingMessages.front();
            // Create a vector of buffers to send:
            // 1. The message header
            // 2. The message body
            std::vector<asio::const_buffer> buffers{
                asio::buffer(&message.m_Header, sizeof(Header<MessageType>)),
                asio::buffer(message.m_Data)};

            // Send the message
            asio::async_write(
                m_socket,
                buffers,
                std::bind(
                    &Connection::HandleWrite,
                    this->shared_from_this(),
                    asio::placeholders::error));
        }

        void HandleWrite(const std::error_code& error)
        {
            if (!error)
            {
                m_OutgoingMessages.pop();
                if (!m_OutgoingMessages.empty())
                {
                    StartWrite();
                }
            }
            else
            {
                std::cerr << "Error while writing to socket: " << error.message() << "\n";
            }
        }

    private:
        uint32_t m_ID{1000};
        tcp::socket m_socket;
        std::queue<Message<MessageType>> m_OutgoingMessages;
        nxs::Server<MessageType> *m_server;
        nxs::Message<MessageType> m_TemporaryMessage;
        nxs::Header<MessageType> m_TemporaryHeader;
    };
}

#endif
