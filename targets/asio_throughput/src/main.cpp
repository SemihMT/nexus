#include <asio.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <thread>
#include <cstring>

using asio::ip::tcp;

const size_t MESSAGE_SIZE = 1024;
const int NUM_MESSAGES = 1024;
const int TEST_AMOUNT = 53;

void handle_read(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<asio::streambuf> buffer, tcp::socket& socket)
{
    if (!error)
    {
        asio::async_write(socket, *buffer, [buffer, &socket](const asio::error_code& write_error, std::size_t)
        {
            if (!write_error)
            {
                // Continue reading
                buffer->consume(buffer->size());
                asio::async_read(socket, *buffer, std::bind(&handle_read, std::placeholders::_1, std::placeholders::_2, buffer, std::ref(socket)));
            }
            else
            {
                std::cerr << "[SERVER] Write error: " << write_error.message() << "\n";
            }
        });
    }
    else if (error == asio::error::eof)
    {
        std::cout << "[SERVER] Client disconnected.\n";
    }
    else
    {
        std::cerr << "[SERVER] Read error: " << error.message() << "\n";
    }
}

void server(asio::io_context& io_context, unsigned short port)
{
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "[SERVER] Listening on port " << port << "...\n";

    tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::cout << "[SERVER] Client connected!\n";

    auto buffer = std::make_shared<asio::streambuf>(MESSAGE_SIZE);
    asio::async_read(socket, *buffer, std::bind(&handle_read, std::placeholders::_1, std::placeholders::_2, buffer, std::ref(socket)));

    io_context.run();
}

void client(const std::string& host, unsigned short port)
{
    asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    tcp::socket socket(io_context);
    asio::connect(socket, endpoints);
    std::cout << "[CLIENT] Connected to server.\n";

    std::vector<char> message(MESSAGE_SIZE, 'A');
    std::vector<double> throughput_values;
    int test_count = 0;

    while (test_count < TEST_AMOUNT)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_MESSAGES; ++i)
        {
            asio::async_write(socket, asio::buffer(message), [&socket, &message](const asio::error_code& error, std::size_t)
            {
                if (!error)
                {
                    asio::async_read(socket, asio::buffer(message), [](const asio::error_code& error, std::size_t)
                    {
                        if (error)
                        {
                            std::cerr << "[CLIENT] Read error: " << error.message() << "\n";
                        }
                    });
                }
                else
                {
                    std::cerr << "[CLIENT] Write error: " << error.message() << "\n";
                }
            });
        }

        io_context.run(); // Execute asynchronous operations

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        double total_data_transferred = static_cast<double>(NUM_MESSAGES * MESSAGE_SIZE) / (1024.0 * 1024.0); // MB
        double throughput = total_data_transferred / duration.count();
        throughput_values.push_back(throughput);

        std::cout << "[CLIENT] Test " << (test_count + 1) << ": Throughput = " << throughput << " MB/s\n";
        ++test_count;
    }

    std::cout << "[CLIENT] All throughput values:\n";
    for (const auto& value : throughput_values)
    {
        std::cout << value << " MB/s\n";
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    try
    {
        if (std::string(argv[1]) == "--server")
        {
            asio::io_context io_context;
            server(io_context, 60000);
        }
        else if (std::string(argv[1]) == "--client")
        {
            client("localhost", 60000);
        }
        else
        {
            std::cerr << "Invalid mode. Use --server or --client\n";
            return 1;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
