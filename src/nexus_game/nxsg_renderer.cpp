#include "nxsg_renderer.h"
#include "nxsg_window.h"
#include <iostream>

namespace nxsg
{
    renderer::renderer(const window &window): m_sdlRenderer(SDL_CreateRenderer(window.GetSDLWindow(), -1, m_rendererFlags))
    {
        if (!m_sdlRenderer)
        {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        }
    }

    renderer::~renderer()
    {
        SDL_DestroyRenderer(m_sdlRenderer);
    }

    void renderer::Clear()
    {
        SDL_RenderClear(m_sdlRenderer);
    }

    void renderer::Present()
    {
        SDL_RenderPresent(m_sdlRenderer);
    }

    void renderer::SetDrawColor(Uint32 r, Uint32 g, Uint32 b, Uint32 a)
    {
        SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, a);
    }
    void renderer::SetDrawColor(Uint32 r, Uint32 g, Uint32 b)
    {
        SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, 255);
    }
    void renderer::SetDrawColor(const SDL_Color &color)
    {
        SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);
    }
} // namespace nxsg