#include "nexus/nexus.h"

namespace nxs
{
    void Nexus::Init(std::size_t threadCount)
    {
        Nexus &instance = Nexus::GetInstance();

        // Default to one thread per logical core. Minimum of 1 so the library
        // is usable even on single-core targets.
        if (threadCount == 0)
            threadCount = std::max(std::size_t{1},
                                   static_cast<std::size_t>(
                                       std::thread::hardware_concurrency()));

        std::cout << "Initializing Nexus with " << threadCount << " worker thread(s).\n";

        instance.m_WorkerThreads.reserve(threadCount);
        for (std::size_t i = 0; i < threadCount; ++i)
        {
            instance.m_WorkerThreads.emplace_back([&instance, i]()
                                                  {
                try
                {
                    instance.m_IOContext.run();
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Exception in Nexus worker thread " << i
                              << ": " << e.what() << "\n";
                } });
        }
    }

    void Nexus::Shutdown()
    {
        Nexus &instance = Nexus::GetInstance();
        std::cout << "Shutting down Nexus...\n";

        // Releasing the work guard lets the context run out of work and stop
        // naturally. Calling stop() directly would abandon queued handlers.
        instance.m_WorkGuard.reset();

        for (auto &t : instance.m_WorkerThreads)
            if (t.joinable())
                t.join();

        instance.m_WorkerThreads.clear();
    }

    asio::io_context &Nexus::GetContext()
    {
        return GetInstance().m_IOContext;
    }

} // namespace nxs
