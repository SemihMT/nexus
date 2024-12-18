#ifndef NEXUS_H
#define NEXUS_H


#include <asio.hpp>
// Windows version definition (versions of windows handle networking differently?)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

#include "singleton.h"
#include <iostream>
namespace nxs
{
  class Nexus final : public Singleton<Nexus>
  {
  public:
    static void Init();
    static void Shutdown();

    asio::io_context &GetContext();

  private:
    friend class Singleton<Nexus>;
    Nexus() {std::cout << "The Nexus constructor has been called!" << "\n";}
    ~Nexus() { Shutdown(); }
    asio::io_context m_IOContext;
    asio::executor_work_guard <decltype(m_IOContext.get_executor())> m_WorkGuard{m_IOContext.get_executor()};
    std::thread m_WorkerThread;
  };
}
#endif // NEXUS_H