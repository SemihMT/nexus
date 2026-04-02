#ifndef CLIENT_H
#define CLIENT_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include "asio.hpp"
#include "message.h"
#include "nexus.h"

namespace nxs
{
  template <typename MessageType>
  class Client final
  {
    static_assert(std::is_enum_v<MessageType>,
                  "MessageType must be an enum class.");
    static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                  "MessageType must have uint32_t as its underlying type.");

    using EventHandler = std::function<void()>;
    using MessageHandler = std::function<void(Message<MessageType> &)>;

  public:
    explicit Client()
        : m_IOContext{Nexus::GetInstance().GetContext()}, m_Socket{m_IOContext}
    {
    }

    ~Client() { Disconnect(); }

    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    // -----------------------------------------------------------------------
    // Fluent API — call before the event loop, returns *this for chaining.
    // -----------------------------------------------------------------------

    /// Initiate an async connection to the server. Triggers OnConnect when ready.
    Client &ConnectTo(const std::string &host, uint16_t port)
    {
      asio::ip::tcp::resolver resolver{m_IOContext};
      auto endpoints = resolver.resolve(host, std::to_string(port));

      asio::async_connect(m_Socket, endpoints,
                          [this](const asio::error_code &error, const asio::ip::tcp::endpoint &)
                          {
                            if (!error)
                              ReceiveClientID();
                            else
                              std::cerr << "Error connecting to server: "
                                        << error.message() << "\n";
                          });
      return *this;
    }

    /// Called once the server has accepted and assigned an ID.
    Client &OnConnect(EventHandler handler)
    {
      m_EventHandlers[Event::Connect] = std::move(handler);
      return *this;
    }

    /// Called when the connection to the server is lost.
    Client &OnDisconnect(EventHandler handler)
    {
      m_EventHandlers[Event::Disconnect] = std::move(handler);
      return *this;
    }

    /// Called when a message of the given type arrives from the server.
    Client &OnMessage(MessageType type, MessageHandler handler)
    {
      m_MessageHandlers[type] = std::move(handler);
      return *this;
    }

    // -----------------------------------------------------------------------
    // Runtime methods
    // -----------------------------------------------------------------------
    void Send(const Message<MessageType> &message)
    {
      bool writeInProgress = !m_OutgoingMessages.empty();
      m_OutgoingMessages.push(message);
      if (!writeInProgress)
        StartWrite();
    }

    void Disconnect()
    {
      if (m_Connected)
      {
        asio::error_code ec;
        m_Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_Socket.close(ec);
        m_Connected = false;
        FireEvent(Event::Disconnect);
      }
    }

    bool IsConnected() const { return m_Connected; }
    uint32_t GetID() const { return m_ID; }
    void SetID(uint32_t id) { m_ID = id; }

  private:
    enum class Event
    {
      Connect,
      Disconnect
    };

    // -----------------------------------------------------------------------
    // Handshake
    // -----------------------------------------------------------------------
    void ReceiveClientID()
    {
      asio::async_read(m_Socket, asio::buffer(&m_ID, sizeof(m_ID)),
                       [this](const asio::error_code &error, std::size_t)
                       {
                         if (!error)
                         {
                           m_Connected = true;
                           FireEvent(Event::Connect);
                           ReceiveHeader();
                         }
                         else
                         {
                           m_Connected = false;
                         }
                       });
    }

    // -----------------------------------------------------------------------
    // Receiving
    // -----------------------------------------------------------------------
    void ReceiveHeader()
    {
      asio::async_read(m_Socket,
                       asio::buffer(&m_TemporaryHeader, sizeof(Header<MessageType>)),
                       [this](const asio::error_code &error, std::size_t)
                       {
                         if (!error)
                         {
                           m_TemporaryMessage.m_Header = m_TemporaryHeader;
                           m_TemporaryMessage.m_Data.resize(m_TemporaryHeader.size);

                           if (m_TemporaryHeader.size > 0)
                             ReceiveBody();
                           else
                           {
                             DispatchMessage(m_TemporaryMessage);
                             ReceiveHeader();
                           }
                         }
                         else
                         {
                           HandleReadError(error);
                         }
                       });
    }

    void ReceiveBody()
    {
      asio::async_read(m_Socket,
                       asio::buffer(m_TemporaryMessage.m_Data.data(),
                                    m_TemporaryMessage.m_Data.size()),
                       [this](const asio::error_code &error, std::size_t)
                       {
                         if (!error)
                         {
                           DispatchMessage(m_TemporaryMessage);
                           ReceiveHeader();
                         }
                         else
                         {
                           HandleReadError(error);
                         }
                       });
    }

    void DispatchMessage(Message<MessageType> &message)
    {
      auto it = m_MessageHandlers.find(message.m_Header.type);
      if (it != m_MessageHandlers.end())
        it->second(message);
      else
        std::cerr << "No handler for message type "
                  << static_cast<uint32_t>(message.m_Header.type) << "\n";
    }

    void HandleReadError(const asio::error_code &error)
    {
      if (error == asio::error::eof ||
          error == asio::error::operation_aborted)
        Disconnect();
      else
        std::cerr << "Read error: " << error.message() << "\n";
    }

    // -----------------------------------------------------------------------
    // Sending
    // -----------------------------------------------------------------------
    void StartWrite()
    {
      if (!m_Socket.is_open() || m_OutgoingMessages.empty())
        return;

      auto &msg = m_OutgoingMessages.front();
      std::vector<asio::const_buffer> buffers{
          asio::buffer(&msg.m_Header, sizeof(Header<MessageType>)),
          asio::buffer(msg.m_Data)};

      asio::async_write(m_Socket, buffers,
                        [this](const asio::error_code &error, std::size_t)
                        {
                          if (!error)
                          {
                            m_OutgoingMessages.pop();
                            if (!m_OutgoingMessages.empty())
                              StartWrite();
                          }
                          else
                          {
                            std::cerr << "Send error: " << error.message() << "\n";
                          }
                        });
    }

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------
    void FireEvent(Event event)
    {
      auto it = m_EventHandlers.find(event);
      if (it != m_EventHandlers.end())
        it->second();
    }

  private:
    bool m_Connected{false};
    asio::io_context &m_IOContext;
    asio::ip::tcp::socket m_Socket;

    std::queue<Message<MessageType>> m_OutgoingMessages;

    Message<MessageType> m_TemporaryMessage{
        static_cast<MessageType>(std::numeric_limits<uint32_t>::max())};
    Header<MessageType> m_TemporaryHeader{};

    std::unordered_map<Event, EventHandler> m_EventHandlers;
    std::unordered_map<MessageType, MessageHandler> m_MessageHandlers;

    uint32_t m_ID{0};
  };

} // namespace nxs
#endif // CLIENT_H
