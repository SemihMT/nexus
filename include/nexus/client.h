#ifndef CLIENT_H
#define CLIENT_H

#include <asio.hpp>
// Windows version definition (versions of windows handle networking differently?)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif
namespace nxs
{
    class Client final
    {
    public:
        explicit Client();
        ~Client();


        void Connect(asio::ip::tcp::endpoint endpoint);
        void Write();
        void Close();

        Client(const Client &other) = delete;
        Client(Client &&other) = delete;
        Client &operator=(const Client &other) = delete;
        Client &operator=(Client &&other) = delete;

    private:
        void HandleConnect(const asio::error_code &error);
        void HandleWrite(const asio::error_code &error);
        void HandleRead(const asio::error_code &error);
        
        asio::io_context &m_IOContext;
        asio::ip::tcp::socket m_Socket;
    };
}
#endif