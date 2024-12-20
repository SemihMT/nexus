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

        void Write();
        void Close();

        Client(const Client &other) = delete;
        Client(Client &&other) = delete;
        Client &operator=(const Client &other) = delete;
        Client &operator=(Client &&other) = delete;

    private:
        void Connect();
        asio::io_context &io_context_;
        asio::ip::tcp::socket socket_;
    };
}
#endif