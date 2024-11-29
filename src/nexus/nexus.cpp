#include "nexus.h"
#include <iostream>

// Windows version definition (versions of windows handle networking differently?)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

// ASIO
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace nxs
{
    void PrintHello()
    {
        std::cout << "Hello from the Nexus API!\n";
    }
}