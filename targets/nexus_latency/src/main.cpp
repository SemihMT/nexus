#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

enum class MyMessageType : uint32_t
{
    Message,
    Reply
};

static nxs::Message<MyMessageType> Make1KBMessage()
{
    nxs::Message<MyMessageType> msg{MyMessageType::Message};
    msg.m_Data.resize(1024, std::byte{'A'});
    msg.m_Header.size = msg.m_Data.size();
    return msg;
}

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
            .OnMessage(MyMessageType::Message, [](auto conn, auto &)
                       { conn->Send(Make1KBMessage()); })
            .RunBlocking();
    }
    else if (std::string(argv[1]) == "--client")
    {
        constexpr int numMessages = 50;
        int count = 0;
        std::vector<long long> rtts;
        std::chrono::steady_clock::time_point sent;

        nxs::Client<MyMessageType> client;

        client
            .OnConnect([&]()
                       {
                std::cout << "[" << client.GetID() << "] Connected. Starting latency test.\n";
                sent = std::chrono::steady_clock::now();
                client.Send(Make1KBMessage());
                ++count; })
            .OnMessage(MyMessageType::Reply, [&](auto &)
                       {
                auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - sent).count();
                rtts.push_back(rtt);

                if (count < numMessages)
                {
                    sent = std::chrono::steady_clock::now();
                    client.Send(Make1KBMessage());
                    ++count;
                }
                else
                {
                    rtts.erase(std::max_element(rtts.begin(), rtts.end()));
                    rtts.erase(std::min_element(rtts.begin(), rtts.end()));
                    long long avg = std::accumulate(rtts.begin(), rtts.end(), 0LL) / (long long)rtts.size();
                    std::cout << "Average RTT (trimmed): " << avg << " µs\n";
                } })
            .ConnectTo("localhost", 60000);

        while (true)
        {
        }
    }

    nxs::Nexus::Shutdown();
    return 0;
}
