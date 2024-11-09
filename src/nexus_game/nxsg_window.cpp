#include "nxsg_window.h"

namespace nxsg
{
    window::window() : m_sdlWindow(SDL_CreateWindow(m_title.c_str(),m_xPos,m_yPos,m_width,m_height,0))
    {
    }

    window::window(const std::string &title, uint32_t width, uint32_t height) : 
    m_title(title), m_width(width), m_height(height), m_sdlWindow(SDL_CreateWindow(m_title.c_str(),m_xPos,m_yPos,m_width,m_height,0))
    {
    }
    window::~window()
    {
        SDL_DestroyWindow(m_sdlWindow);
    }
}