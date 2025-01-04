#ifndef CLIENT_H
#define CLIENT_H

// Windows version definition (versions of windows handle networking differently?)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif
#include <asio.hpp>
#include "message.h"
#include "nexus.h"
namespace nxs
{
    template <typename MessageType>
    class Client final
    {
        // Test for MessageType being an enum class and having uint32_t as its underlying type
        static_assert(std::is_enum_v<MessageType>, "MessageType must be an enum class.");
        static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                      "MessageType must have uint32_t as its underlying type.");

    public:
        explicit Client()
            : m_IOContext{Nexus::GetInstance().GetContext()}, m_Socket{m_IOContext}
        {
        };
        ~Client()
        {
            Disconnect();
        };

        // Connect to a server
        bool Connect(const std::string &host, uint16_t port)
        {
            // Resolve the host and port
            asio::ip::tcp::resolver resolver{m_IOContext};
            auto endpoints = resolver.resolve(host, std::to_string(port));
            // Connect to the server
            asio::async_connect(m_Socket, endpoints,
                [this](const asio::error_code &error, const asio::ip::tcp::endpoint &endpoint)
                {
                    if (!error)
                    {
                        ReceiveClientID(); // Read the server-assigned client ID
                    }
                    else
                    {
                        std::cerr << "Error connecting to server: " << error.message() << "\n";
                    }
                });
            return m_Connected;
        };
        // Disconnect from the server
        void Disconnect()
        {
            if (m_Connected)
            {
                m_Socket.close();
                m_Connected = false;
            }
        };
        // Send a Message (see message.h) to the server
        void Send(const Message<MessageType>& message)
        {
            SendHeader(message.m_Header);
            SendBody(message.m_Data);
        };

        // Add Message handler for a specific message type
        void AddMessageHandler(MessageType type, std::function<void(Message<MessageType>&)> handler)
        {
            m_MessageHandlers[type] = handler;
        };
        // Process incoming messages
        void ProcessIncomingMessages()
        {
            while (!m_IncomingMessages.empty())
            {
                auto& message = m_IncomingMessages.front();
                auto it = m_MessageHandlers.find(message.m_Header.type);
                if (it != m_MessageHandlers.end())
                {
                    it->second(message);
                }
                else
                {
                    std::cerr << "No handler registered for message type: "
                              << static_cast<uint32_t>(message.m_Header.type) << "\n";
                }
                m_IncomingMessages.pop();
            }
        };
        void SetID(uint32_t id)
        {
            m_ID = id;
        };
        uint32_t GetID() const
        {
            return m_ID;
        };
        Client(const Client &other) = delete;
        Client(Client &&other) = delete;
        Client &operator=(const Client &other) = delete;
        Client &operator=(Client &&other) = delete;

    private:
    void ReceiveClientID()
    {
        asio::async_read(
            m_Socket,
            asio::buffer(&m_ID, sizeof(m_ID)),
            [this](const asio::error_code &error, std::size_t /*length*/)
            {
                HandleConnect(error);
            });
    }
    void HandleConnect(const asio::error_code &error)
    {
        if (!error)
        {
            m_Connected = true;
            // Start receiving messages
            ReceiveHeader();
        }
        else
        {
            m_Connected = false;
        }
    };

     void ReceiveHeader()
        {
            asio::async_read(
                m_Socket,
                asio::buffer(&m_TemporaryHeader, sizeof(Header<MessageType>)),
                [this](const asio::error_code &error, std::size_t /*length*/) {
                    if (!error)
                    {
                        // Resize the temporary message body based on the size from the header
                        m_TemporaryMessage.m_Header = m_TemporaryHeader;
                        m_TemporaryMessage.m_Data.resize(m_TemporaryHeader.size);

                        if (m_TemporaryHeader.size > 0)
                        {
                            // Read the message body
                            ReceiveBody();
                        }
                        else
                        {
                            // No body, process the message immediately
                            OnMessageReceived();
                            ReceiveHeader(); // Continue reading the next message
                        }
                    }
                    else
                    {
                        HandleReadError(error);
                    }
                });
        };

        void ReceiveBody()
        {
            asio::async_read(
                m_Socket,
                asio::buffer(m_TemporaryMessage.m_Data.data(), m_TemporaryMessage.m_Data.size()),
                [this](const asio::error_code &error, std::size_t /*length*/) {
                    if (!error)
                    {
                        // Entire message has been received
                        OnMessageReceived();
                        ReceiveHeader(); // Continue reading the next message
                    }
                    else
                    {
                        HandleReadError(error);
                    }
                });
        };

        void OnMessageReceived()
        {
            // Add the received message to the incoming queue
            m_IncomingMessages.push(m_TemporaryMessage);
            // Process the message (optional: can be done in a separate thread or loop)
            ProcessIncomingMessages();
        };

        void HandleReadError(const asio::error_code &error)
        {
            if (error == asio::error::eof || error == asio::error::operation_aborted)
            {
                // Connection closed by server or aborted
                Disconnect();
            }
            else
            {
                std::cerr << "Error reading from socket: " << error.message() << "\n";
            }
        };

        void SendHeader(const Header<MessageType>& header)
        {
            asio::async_write(
                m_Socket,
                asio::buffer(&header, sizeof(Header<MessageType>)),
                [this](const asio::error_code &error, std::size_t /*length*/) {
                    if (error)
                    {
                        std::cerr << "Error sending header: " << error.message() << "\n";
                    }
                });
        };

        void SendBody(const std::vector<std::byte>& body)
        {
            asio::async_write(
                m_Socket,
                asio::buffer(body.data(), body.size()),
                [this](const asio::error_code &error, std::size_t /*length*/) {
                    if (error)
                    {
                        std::cerr << "Error sending body: " << error.message() << "\n";
                    }
                });
        };
       
    private:
        bool m_Connected{false};
        asio::io_context &m_IOContext;
        asio::ip::tcp::socket m_Socket;

        // Client should have 2 queues, one for sending and one for receiving
        std::queue<Message<MessageType>> m_OutgoingMessages;
        std::queue<Message<MessageType>> m_IncomingMessages;

         // Temporary message and header for receiving
        Message<MessageType> m_TemporaryMessage{static_cast<MessageType>(std::numeric_limits<uint32_t>::max())};
        Header<MessageType> m_TemporaryHeader{};

        // Map of message handlers
        std::unordered_map<MessageType, std::function<void(Message<MessageType>&)>> m_MessageHandlers;

        // Client ID (assigned by the server)
        uint32_t m_ID{0};
    }; 
} // namespace nxs
#endif