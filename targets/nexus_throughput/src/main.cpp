#include <nexus/nexus.h>
#include <nexus/server.h>
#include <nexus/client.h>
#include <chrono>
#include <iostream>
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
            .OnMessage(MyMessageType::Message, [](auto conn, auto &msg)
                       {
                nxs::Message<MyMessageType> reply{MyMessageType::Reply};
                reply.m_Data.resize(1024, std::byte{'A'});
                reply.m_Header.size = reply.m_Data.size();
                conn->Send(reply); })
            .RunBlocking();
    }
    else if (std::string(argv[1]) == "--client")
    {
        constexpr int numMessages = 1024;
        constexpr int numTests = 50;
        int received = 0;
        int testsDone = 0;
        std::vector<double> throughputs;
        std::chrono::steady_clock::time_point batchStart;
        const auto msgSize = Make1KBMessage().Size();

        auto sendBatch = [&]()
        {
            batchStart = std::chrono::steady_clock::now();
            auto msg = Make1KBMessage();
            for (int i = 0; i < numMessages; ++i)
                // client captured by ref in lambda below — send from there
                ;
            // returned so the OnConnect/OnMessage lambdas can use it
        };

        nxs::Client<MyMessageType> client;

        client
            .OnConnect([&]()
                       {
                std::cout << "[" << client.GetID() << "] Connected. Starting throughput test.\n";
                batchStart = std::chrono::steady_clock::now();
                auto msg = Make1KBMessage();
                for (int i = 0; i < numMessages; ++i)
                    client.Send(msg);
                ++testsDone; })
            .OnMessage(MyMessageType::Reply, [&](auto &)
                       {
                if (++received < numMessages) return;

                // Batch complete
                received = 0;
                std::chrono::duration<double> elapsed =
                    std::chrono::steady_clock::now() - batchStart;
                double mb         = static_cast<double>(numMessages * msgSize) / (1024.0 * 1024.0);
                double throughput = mb / elapsed.count();
                throughputs.push_back(throughput);
                std::cout << "Test " << testsDone << ": " << throughput << " MB/s\n";

                if (testsDone < numTests)
                {
                    batchStart = std::chrono::steady_clock::now();
                    auto msg = Make1KBMessage();
                    for (int i = 0; i < numMessages; ++i)
                        client.Send(msg);
                    ++testsDone;
                }
                else
                {
                    std::cout << "\nAll results:\n";
                    for (double t : throughputs)
                        std::cout << "  " << t << " MB/s\n";
                } })
            .ConnectTo("localhost", 60000);

        while (true)
        {
        }
    }

    nxs::Nexus::Shutdown();
    return 0;
}
