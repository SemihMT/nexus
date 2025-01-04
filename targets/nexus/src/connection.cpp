// #include "connection.h"
// #include "server.h"
// namespace nxs
// {
//     using asio::ip::tcp;
//     std::shared_ptr<Connection> Connection::Create()
//     {
//         auto sharedConnectionPtr = std::shared_ptr<Connection>(new Connection(Nexus::GetInstance().GetContext()));
//         return sharedConnectionPtr;
//     }
//     tcp::socket &Connection::socket()
//     {
//         return m_socket;
//     }

//     void Connection::Start()
//     {
//         // Start listening for incoming messages
//         StartRead();

//         StartWrite();
//     }

//     void Connection::Send(const std::string &message)
//     {
//         m_message = message;
//         StartWrite();
//     }

//     void Connection::SetServer(nxs::Server *server)
//     {
//         m_server = server;
//     }
//     Connection::Connection(asio::io_context &ctx) : m_socket{ctx} {}

//     void Connection::StartRead()
//     {
//         // Clear the buffer before starting the read operation
//         m_buffer.fill(0); // Assuming m_buffer is a std::array
//         m_socket.async_read_some(
//             asio::buffer(m_buffer),
//             std::bind(
//                 &Connection::HandleRead,
//                 shared_from_this(),
//                 asio::placeholders::error,
//                 asio::placeholders::bytes_transferred));
//     }

//     void Connection::HandleRead(const std::error_code &error, std::size_t bytesTransferred)
//     {
//         if (!error)
//         {
//             // Process the received data
//             std::string message(m_buffer.data(), bytesTransferred);
//             std::cout << "Message from client: " << message << "\n";

//             // Continue reading
//             StartRead();
//         }
//         else if (error == asio::error::eof)
//         {
//             std::cerr << "Connection closed by the client.\n";
//             m_server->OnConnectionDisconnected(shared_from_this());
//         }
//         else if (error == asio::error::operation_aborted)
//         {
//             std::cerr << "Operation aborted.\n";
//         }
//         else
//         {
//             std::cerr << "Error while reading message: " << error.message() << "\n";
//         }
//     }

//     void Connection::StartWrite()
//     {
//         if (!m_message.empty())
//         {

//             asio::async_write(
//                 m_socket,
//                 asio::buffer(m_message),
//                 std::bind(
//                     &Connection::HandleWrite,
//                     shared_from_this(),
//                     asio::placeholders::error,
//                     asio::placeholders::bytes_transferred));
//         }
//     }

//     void Connection::HandleWrite(const std::error_code &error, size_t bytesTransferred)
//     {
//         if (!error)
//         {
//             std::cout << "Number of bytes transferred: [" << bytesTransferred << "]\n";
//             m_message.clear();
//             // Continue the message-sending loop
//             StartWrite();
//         }
//         else if (error == asio::error::eof)
//         {
//             std::cerr << "Connection closed by the client.\n";
//             m_server->OnConnectionDisconnected(shared_from_this());
//         }
//         else if (error == asio::error::operation_aborted)
//         {
//             std::cerr << "Operation aborted.\n";
//         }
//         else
//         {
//             std::cerr << "Error while sending message: " << error.message() << "\n";
//         }
//     }
// } // namespace nxs