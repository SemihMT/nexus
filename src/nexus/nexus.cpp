#include "nexus.h"

namespace nxs
{
    void Nexus::Init()
    {
        std::cout << "Initializing the Nexus Library!" << "\n";
        // Explicitly initialize the Nexus library
        Nexus &instance = Nexus::GetInstance();
        instance.m_WorkerThread = std::thread{[&](){
            try
            {
                instance.m_IOContext.run();
            }
            catch(const std::exception &e)
            {
                std::cerr << "Exception in Nexus worker thread: " << e.what() << "\n";
            }
        }};
    }

    
    void Nexus::Shutdown()
    {
        // Will do any cleanup code here
        Nexus &instance = GetInstance();
        std::cout << "Shutting down Nexus...\n";

        // Stop the IO context
        instance.m_IOContext.stop();

        // Join the worker thread if it is joinable
        if (instance.m_WorkerThread.joinable())
        {
            instance.m_WorkerThread.join();
        }

        // Reset the work guard to release the executor
        instance.m_WorkGuard.reset();
    }

    asio::io_context &Nexus::GetContext()
    {
        // Return the context object for fine grained control
        return GetInstance().m_IOContext;
    }
}