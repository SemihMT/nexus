#include <asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring> // For memcpy

const int port = 60000;

void run_server()
{
    asio::io_context io_context;

    asio::ip::tcp::acceptor acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
    std::cout << "[SERVER] Running on port " << port << "...\n";

    std::vector<std::thread> threads;

    while (true)
    {
        asio::ip::tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::cout << "[SERVER] Client connected: " << socket.remote_endpoint().address() << "\n";

        threads.emplace_back([socket = std::move(socket)]() mutable {
            try
            {
                while (true)
                {
                    char data[1024];
                    asio::error_code error;

                    size_t length = socket.read_some(asio::buffer(data), error);
                    if (error == asio::error::eof)
                    {
                        std::cout << "[SERVER] Client disconnected.\n";
                        break;
                    }
                    else if (error)
                    {
                        throw asio::system_error(error);
                    }

                    std::cout << "[SERVER] Received  message of size " << length << " bytes.\n";

                    char reply[1024];
                    std::memset(reply, 'A', sizeof(reply));
                    asio::write(socket, asio::buffer(reply, sizeof(reply)), error);

                    if (error)
                    {
                        throw asio::system_error(error);
                    }

                    std::cout << "[SERVER] Sent reply.\n";
                }
            }
            catch (std::exception &e)
            {
                std::cerr << "[SERVER] Exception: " << e.what() << "\n";
            }
        });
    }

    for (auto &thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
}

void run_client()
{
    asio::io_context io_context;
    asio::ip::tcp::resolver resolver(io_context);
    asio::ip::tcp::socket socket(io_context);

    auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
    asio::connect(socket, endpoints);

    std::cout << "[CLIENT] Connected to server.\n";

    int messageCount = 0;
    int numMessages = 50;
    std::vector<long long> roundTripTimes;
    std::chrono::high_resolution_clock::time_point start, end;

    char message[1024];
    std::memset(message, 'A', sizeof(message));

    while (messageCount < numMessages)
    {
        start = std::chrono::high_resolution_clock::now();

        asio::write(socket, asio::buffer(message, sizeof(message)));

        char reply[1024];
        asio::error_code error;
        size_t length = asio::read(socket, asio::buffer(reply, sizeof(reply)), error);

        if (error == asio::error::eof)
        {
            std::cerr << "[CLIENT] Server disconnected.\n";
            break;
        }
        else if (error)
        {
            throw asio::system_error(error);
        }

        end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        roundTripTimes.push_back(duration.count());

        std::cout << "[CLIENT] Received reply of size " << length << " bytes.\n";
        std::cout << "Round trip time: " << duration.count() << "\n";

        messageCount++;
    }

    // Print round trip times
    std::cout << "Round trip times:\n";
    for (auto time : roundTripTimes)
    {
        std::cout << time << "\n";
    }

    roundTripTimes.erase(std::max_element(roundTripTimes.begin(), roundTripTimes.end()));
    roundTripTimes.erase(std::min_element(roundTripTimes.begin(), roundTripTimes.end()));
    long long sum = std::accumulate(roundTripTimes.begin(), roundTripTimes.end(), 0LL);
    long long average = sum / roundTripTimes.size();
    std::cout << "Average round trip time: " << average << " us\n";
}

int main(int argc, char *argv[])
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
            run_server();
        }
        else if (std::string(argv[1]) == "--client")
        {
            run_client();
        }
        else
        {
            std::cerr << "Invalid mode. Use --server or --client\n";
            return 1;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
