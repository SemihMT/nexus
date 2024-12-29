#include "client.h"
#include "nexus.h"
namespace nxs
{
    Client::Client() : m_IOContext{Nexus::GetInstance().GetContext()}, m_Socket{m_IOContext}
    {
    }
} // namespace nxs
