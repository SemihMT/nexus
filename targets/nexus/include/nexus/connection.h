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
        void Disconnect()
        {
            if(m_socket.is_open())
            {
                // Stop any ongoing reads or writes
                m_socket.shutdown(tcp::socket::shutdown_both);
                //Close the socket
                m_socket.close();
                if(m_server)
                {
                    m_server->OnConnectionDisconnected(this->shared_from_this());
                }
                std::cout << "[" << m_ID << "] Disconnected\n";
            }
        }
        void Send(const Message<MessageType> &message)
        {
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
            if(!m_socket.is_open())
            {
                return;
            }
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
            if(!m_socket.is_open())
            {
                return;
            }
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
        void OnMessageReceived()
        {
            // Add to incoming queue
            m_server->ProcessIncomingMessage(this->shared_from_this(), m_TemporaryMessage);
        }

        void HandleReadError(const std::error_code &error)
        {
            // NOTE: This function is called when an error occurs while reading from the socket
            // When any error occurs, we now immediately disconnect 
            // This is a simple way to handle errors, needs more sophisticated error handling
            if (error == asio::error::eof)
            {
                std::cerr << "Error reading from socket: " << error.message() << "\n";
                Disconnect();
            }
            else if (error == asio::error::operation_aborted)
            {
                std::cerr << "Error reading from socket: " << error.message() << "\n";
                Disconnect();
            }
            else
            {
                std::cerr << "Error reading from socket: " << error.message() << "\n";
                Disconnect();
            }
        }
        void StartWrite()
        {
            if (!m_socket.is_open())
            {
                return;
            }
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
