#ifndef CLIENT_H
#define CLIENT_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#include "asio.hpp"
#include "message.h"
#include "nexus.h"


namespace nxs {
template <typename MessageType> class Client final {
  static_assert(std::is_enum_v<MessageType>,
                "MessageType must be an enum class.");
  static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                "MessageType must have uint32_t as its underlying type.");

public:
  enum class ClientEvent { OnConnect, OnDisconnect, OnMessage };

public:
  explicit Client()
      : m_IOContext{Nexus::GetInstance().GetContext()}, m_Socket{m_IOContext} {}

  ~Client() { Disconnect(); }

  void Connect(const std::string &host, uint16_t port) {
    asio::ip::tcp::resolver resolver{m_IOContext};
    auto endpoints = resolver.resolve(host, std::to_string(port));

    asio::async_connect(
        m_Socket, endpoints,
        [this](const asio::error_code &error, const asio::ip::tcp::endpoint &) {
          if (!error) {
            ReceiveClientID();
          } else {
            std::cerr << "Error connecting to server: " << error.message()
                      << "\n";
          }
        });
  }

  bool IsConnected() { return m_Connected; }

  void Disconnect() {
    if (m_Connected) {
      m_Socket.shutdown(asio::ip::tcp::socket::shutdown_both);
      m_Socket.close();
      m_Connected = false;
      CallEventHandler(ClientEvent::OnDisconnect);
    }
  }

  // Queue a message for sending. Header and body are written as a single
  // vectored async_write so they can never be interleaved by a concurrent
  // Send().
  void Send(const Message<MessageType> &message) {
    // If a write is already in progress, just enqueue — StartWrite() will
    // drain the queue when the current write completes.
    bool writeInProgress = !m_OutgoingMessages.empty();
    m_OutgoingMessages.push(message);
    if (!writeInProgress) {
      StartWrite();
    }
  }

  void AddEventHandler(ClientEvent event, std::function<void()> handler) {
    m_EventHandlers[event] = handler;
  }

  void AddMessageHandler(MessageType type,
                         std::function<void(Message<MessageType> &)> handler) {
    m_MessageHandlers[type] = handler;
  }

  void ProcessIncomingMessages() {
    while (!m_IncomingMessages.empty()) {
      auto &message = m_IncomingMessages.front();
      auto it = m_MessageHandlers.find(message.m_Header.type);
      if (it != m_MessageHandlers.end()) {
        it->second(message);
      } else {
        std::cerr << "No handler registered for message type: "
                  << static_cast<uint32_t>(message.m_Header.type) << "\n";
      }
      m_IncomingMessages.pop();
    }
  }

  void SetID(uint32_t id) { m_ID = id; }
  uint32_t GetID() const { return m_ID; }

  Client(const Client &) = delete;
  Client(Client &&) = delete;
  Client &operator=(const Client &) = delete;
  Client &operator=(Client &&) = delete;

private:
  // -----------------------------------------------------------------------
  // Connection handshake
  // -----------------------------------------------------------------------
  void ReceiveClientID() {
    asio::async_read(m_Socket, asio::buffer(&m_ID, sizeof(m_ID)),
                     [this](const asio::error_code &error, std::size_t) {
                       HandleConnect(error);
                     });
  }

  void HandleConnect(const asio::error_code &error) {
    if (!error) {
      m_Connected = true;
      CallEventHandler(ClientEvent::OnConnect);
      ReceiveHeader();
    } else {
      m_Connected = false;
    }
  }

  // -----------------------------------------------------------------------
  // Receiving
  // -----------------------------------------------------------------------
  void ReceiveHeader() {
    asio::async_read(
        m_Socket, asio::buffer(&m_TemporaryHeader, sizeof(Header<MessageType>)),
        [this](const asio::error_code &error, std::size_t) {
          if (!error) {
            m_TemporaryMessage.m_Header = m_TemporaryHeader;
            m_TemporaryMessage.m_Data.resize(m_TemporaryHeader.size);

            if (m_TemporaryHeader.size > 0)
              ReceiveBody();
            else {
              OnMessageReceived();
              ReceiveHeader();
            }
          } else {
            HandleReadError(error);
          }
        });
  }

  void ReceiveBody() {
    asio::async_read(m_Socket,
                     asio::buffer(m_TemporaryMessage.m_Data.data(),
                                  m_TemporaryMessage.m_Data.size()),
                     [this](const asio::error_code &error, std::size_t) {
                       if (!error) {
                         OnMessageReceived();
                         ReceiveHeader();
                       } else {
                         HandleReadError(error);
                       }
                     });
  }

  void OnMessageReceived() {
    m_IncomingMessages.push(m_TemporaryMessage);
    ProcessIncomingMessages();
  }

  void HandleReadError(const asio::error_code &error) {
    if (error == asio::error::eof || error == asio::error::operation_aborted) {
      Disconnect();
    } else {
      std::cerr << "Error reading from socket: " << error.message() << "\n";
    }
  }

  // -----------------------------------------------------------------------
  // Sending — queue-based, single vectored write per message
  // -----------------------------------------------------------------------
  void StartWrite() {
    if (!m_Socket.is_open() || m_OutgoingMessages.empty())
      return;

    auto &message = m_OutgoingMessages.front();

    // Send header + body in one async_write so they are guaranteed to
    // be contiguous on the wire and can never be split by another Send().
    std::vector<asio::const_buffer> buffers{
        asio::buffer(&message.m_Header, sizeof(Header<MessageType>)),
        asio::buffer(message.m_Data)};

    asio::async_write(m_Socket, buffers,
                      [this](const asio::error_code &error, std::size_t) {
                        HandleWrite(error);
                      });
  }

  void HandleWrite(const asio::error_code &error) {
    if (!error) {
      m_OutgoingMessages.pop();
      // Keep draining the queue if more messages were enqueued
      // while this write was in flight.
      if (!m_OutgoingMessages.empty())
        StartWrite();
    } else {
      std::cerr << "Error sending message: " << error.message() << "\n";
    }
  }

  // -----------------------------------------------------------------------
  // Event/message dispatch
  // -----------------------------------------------------------------------
  void CallEventHandler(ClientEvent event) {
    auto it = m_EventHandlers.find(event);
    if (it != m_EventHandlers.end())
      it->second();
  }

private:
  bool m_Connected{false};
  asio::io_context &m_IOContext;
  asio::ip::tcp::socket m_Socket;

  std::queue<Message<MessageType>> m_OutgoingMessages; // now actually used
  std::queue<Message<MessageType>> m_IncomingMessages;

  Message<MessageType> m_TemporaryMessage{
      static_cast<MessageType>(std::numeric_limits<uint32_t>::max())};
  Header<MessageType> m_TemporaryHeader{};

  std::unordered_map<ClientEvent, std::function<void()>> m_EventHandlers;
  std::unordered_map<MessageType, std::function<void(Message<MessageType> &)>>
      m_MessageHandlers;

  uint32_t m_ID{0};
};

} // namespace nxs
#endif
