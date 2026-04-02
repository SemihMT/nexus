#ifndef NEXUS_H
#define NEXUS_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include "singleton.h"
#include <iostream>
#include <queue>
#include <thread>
#include <vector>

namespace nxs
{
  class Nexus final : public Singleton<Nexus>
  {
  public:
    static void Init(std::size_t threadCount = 0);
    static void Shutdown();

    asio::io_context &GetContext();

  private:
    friend class Singleton<Nexus>;

    Nexus() = default;

    // Shutdown() is called explicitly by the user. The destructor must NOT
    // call it via GetInstance() — that is undefined behaviour on a static
    // that is already being destroyed. Instead, the destructor only cleans
    // up state it directly owns, and only if Init() was ever called.
    ~Nexus()
    {
      // Stop the context and drain remaining threads if the user forgot
      // to call Shutdown() explicitly.
      if (!m_WorkerThreads.empty())
      {
        m_IOContext.stop();
        for (auto &t : m_WorkerThreads)
          if (t.joinable())
            t.join();
      }
    }

    asio::io_context m_IOContext;

    // Work guard keeps the context alive even when the completion queue
    // is momentarily empty, so threads don't exit prematurely.
    asio::executor_work_guard<decltype(m_IOContext.get_executor())>
        m_WorkGuard{m_IOContext.get_executor()};

    // One thread per logical core (configurable via Init).
    std::vector<std::thread> m_WorkerThreads;
  };

} // namespace nxs

#endif // NEXUS_H
