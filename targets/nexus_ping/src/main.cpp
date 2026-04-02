#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <chrono>
#include <iostream>

enum class MyMessageType : uint32_t
{
    Ping,
    Pong
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --server | --client\n";
        return 1;
    }

    nxs::Nexus::Init();

    if (std::string(argv[1]) == "--server")
    {
        nxs::Server<MyMessageType> server;

        server
            .OnConnect([](auto conn)
                       { std::cout << "[SERVER] Client [" << conn->GetID() << "] connected.\n"; })
            .OnMessage(MyMessageType::Ping, [](auto conn, auto &)
                       {
                std::cout << "[SERVER] Ping from [" << conn->GetID() << "]. Sending Pong.\n";
                nxs::Message<MyMessageType> pong{MyMessageType::Pong};
                conn->Send(pong); })
            .RunBlocking();
    }
    else if (std::string(argv[1]) == "--client")
    {
        std::chrono::steady_clock::time_point pingSent;
        nxs::Client<MyMessageType> client;

        client
            .OnConnect([&]()
                       {
                std::cout << "[" << client.GetID() << "] Connected. Sending Ping.\n";
                nxs::Message<MyMessageType> ping{MyMessageType::Ping};
                client.Send(ping);
                pingSent = std::chrono::steady_clock::now(); })
            .OnMessage(MyMessageType::Pong, [&](auto &)
                       {
                auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - pingSent);
                std::cout << "[" << client.GetID() << "] Pong received. RTT: "
                          << rtt.count() << " µs\n"; })
            .ConnectTo("localhost", 60000);

        while (true)
        {
        }
    }

    nxs::Nexus::Shutdown();
    return 0;
}
