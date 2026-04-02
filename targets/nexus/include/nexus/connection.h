#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <memory>
#include <asio.hpp>
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
        // -----------------------------------------------------------------------
        // Factory — private constructor, force shared_ptr ownership
        // -----------------------------------------------------------------------
        static std::shared_ptr<Connection<MessageType>> Create()
        {
            return std::shared_ptr<Connection<MessageType>>(
                new Connection<MessageType>(Nexus::GetInstance().GetContext()));
        }

        // -----------------------------------------------------------------------
        // Public interface
        // -----------------------------------------------------------------------
        tcp::socket &socket() { return m_Socket; }

        // Exposed so Server can bind the initial client-ID write through this
        // connection's strand before Start() is called.
        asio::strand<asio::io_context::executor_type> &strand() { return m_Strand; }

        void Start()
        {
            StartRead();
        }

        void Disconnect()
        {
            // Post through the strand so this is safe to call from any thread.
            // The actual close happens on the strand, preventing a race with
            // an in-progress read or write handler.
            asio::post(m_Strand, [self = this->shared_from_this()]()
                       { self->DoDisconnect(); });
        }

        void Send(const Message<MessageType> &message)
        {
            // Posting the enqueue+kick through the strand means Send() is safe
            // to call from any thread (including the game loop on the main thread)
            // without an external mutex.
            asio::post(m_Strand, [self = this->shared_from_this(), message]()
                       {
                bool writeInProgress = !self->m_OutgoingMessages.empty();
                self->m_OutgoingMessages.push(message);
                if (!writeInProgress)
                    self->StartWrite(); });
        }

        void SetServer(nxs::Server<MessageType> *server) { m_Server = server; }
        void SetID(uint32_t id) { m_ID = id; }
        uint32_t GetID() const { return m_ID; }

    private:
        // -----------------------------------------------------------------------
        // Construction
        // -----------------------------------------------------------------------
        explicit Connection(asio::io_context &ctx)
            : m_Socket{ctx}, m_Strand{asio::make_strand(ctx)}, m_TemporaryMessage{static_cast<MessageType>(
                                                                   std::numeric_limits<uint32_t>::max())}
        {
        }

        // -----------------------------------------------------------------------
        // Disconnect (must always run on the strand)
        // -----------------------------------------------------------------------
        void DoDisconnect()
        {
            if (!m_Socket.is_open())
                return;

            asio::error_code ec;
            m_Socket.shutdown(tcp::socket::shutdown_both, ec);
            m_Socket.close(ec);

            if (m_Server)
                m_Server->OnConnectionDisconnected(this->shared_from_this());

            std::cout << "[" << m_ID << "] Disconnected\n";
        }

        // -----------------------------------------------------------------------
        // Reading — all handlers bound to the strand
        // -----------------------------------------------------------------------
        void StartRead()
        {
            if (!m_Socket.is_open())
                return;

            asio::async_read(
                m_Socket,
                asio::buffer(&m_TemporaryHeader, sizeof(Header<MessageType>)),
                asio::bind_executor(m_Strand,
                                    std::bind(&Connection::HandleReadHeader,
                                              this->shared_from_this(),
                                              asio::placeholders::error)));
        }

        void HandleReadHeader(const asio::error_code &error)
        {
            if (!m_Socket.is_open())
                return;

            if (!error)
            {
                if (m_TemporaryHeader.size > MAX_MESSAGE_BODY)
                {
                    std::cerr << "[" << m_ID << "] Claimed body size "
                              << m_TemporaryHeader.size
                              << " bytes exceeds MAX_MESSAGE_BODY ("
                              << MAX_MESSAGE_BODY << "). Disconnecting.\n";
                    DoDisconnect();
                    return;
                }

                m_TemporaryMessage.m_Header = m_TemporaryHeader;
                m_TemporaryMessage.m_Data.resize(m_TemporaryHeader.size);

                if (m_TemporaryHeader.size > 0)
                {
                    asio::async_read(
                        m_Socket,
                        asio::buffer(m_TemporaryMessage.m_Data.data(),
                                     m_TemporaryHeader.size),
                        asio::bind_executor(m_Strand,
                                            std::bind(&Connection::HandleReadBody,
                                                      this->shared_from_this(),
                                                      asio::placeholders::error)));
                }
                else
                {
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
                OnMessageReceived();
                StartRead();
            }
            else
            {
                HandleReadError(error);
            }
        }

        void OnMessageReceived()
        {
            if (m_Server)
                m_Server->ProcessIncomingMessage(
                    this->shared_from_this(), m_TemporaryMessage);
        }

        void HandleReadError(const asio::error_code &error)
        {
            if (error != asio::error::operation_aborted)
                std::cerr << "[" << m_ID << "] Read error: "
                          << error.message() << "\n";
            DoDisconnect();
        }

        // -----------------------------------------------------------------------
        // Writing — all handlers bound to the strand
        // -----------------------------------------------------------------------
        void StartWrite()
        {
            if (!m_Socket.is_open() || m_OutgoingMessages.empty())
                return;

            auto &message = m_OutgoingMessages.front();

            std::vector<asio::const_buffer> buffers{
                asio::buffer(&message.m_Header, sizeof(Header<MessageType>)),
                asio::buffer(message.m_Data)};

            asio::async_write(
                m_Socket,
                buffers,
                asio::bind_executor(m_Strand,
                                    std::bind(&Connection::HandleWrite,
                                              this->shared_from_this(),
                                              asio::placeholders::error)));
        }

        void HandleWrite(const asio::error_code &error)
        {
            if (!error)
            {
                m_OutgoingMessages.pop();
                if (!m_OutgoingMessages.empty())
                    StartWrite();
            }
            else
            {
                std::cerr << "[" << m_ID << "] Write error: "
                          << error.message() << "\n";
            }
        }

        // -----------------------------------------------------------------------
        // Data members
        // -----------------------------------------------------------------------

        // Maximum allowed body size per message. A client claiming a larger body
        // is either malicious or buggy — disconnect it immediately rather than
        // attempting the allocation. Override at compile time via -DNXS_MAX_MESSAGE_BODY=N
        // if your game needs larger messages (e.g. initial world-state transfers).
#ifndef NXS_MAX_MESSAGE_BODY
        static constexpr uint32_t MAX_MESSAGE_BODY = 4u * 1024u * 1024u; // 4 MB
#else
        static constexpr uint32_t MAX_MESSAGE_BODY = NXS_MAX_MESSAGE_BODY;
#endif

        uint32_t m_ID{1000};
        tcp::socket m_Socket;

        // The strand serialises all handlers for THIS connection while allowing
        // handlers from OTHER connections to run concurrently on the thread pool.
        // No external mutex is needed for m_OutgoingMessages, m_TemporaryMessage,
        // or m_TemporaryHeader because they are only ever touched on the strand.
        asio::strand<asio::io_context::executor_type> m_Strand;

        std::queue<Message<MessageType>> m_OutgoingMessages;

        nxs::Message<MessageType> m_TemporaryMessage;
        nxs::Header<MessageType> m_TemporaryHeader{};

        nxs::Server<MessageType> *m_Server{nullptr};
    };

} // namespace nxs

#endif // CONNECTION_H
