#ifndef NXSG_WINDOW_H
#define NXSG_WINDOW_H

#include <string>
#include <SDL.h>

namespace nxsg
{
    class window final
    {
    public:
        window();
        window(const std::string &title, uint32_t width, uint32_t height);
        ~window();

        window(const window &) = delete;
        window(window &&) = delete;
        window &operator=(const window &) = delete;
        window &operator=(window &&) = delete;
    public:
        SDL_Window* GetSDLWindow() const {return m_sdlWindow;}

    private:
        std::string m_title{"Window"};
        uint32_t m_xPos{SDL_WINDOWPOS_CENTERED};
        uint32_t m_yPos{SDL_WINDOWPOS_CENTERED};
        uint32_t m_width{720};
        uint32_t m_height{480};
        SDL_Window* m_sdlWindow{nullptr};
    };
}

#endif //