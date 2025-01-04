#ifndef NXSG_RENDERER_H
#define NXSG_RENDERER_H

#include <SDL.h>

namespace nxsg
{
    class window;
    class renderer final
    {
        public:
        renderer(const nxsg::window& window);
        ~renderer();

        renderer(const renderer&) = delete;
        renderer(renderer&&) = delete;
        renderer& operator=(const renderer&) = delete;
        renderer& operator=(renderer&&) = delete;
        public:
        void Clear();
        void Present();
        
        void SetDrawColor(Uint32 r,Uint32 g,Uint32 b,Uint32 a);
        void SetDrawColor(Uint32 r,Uint32 g,Uint32 b);
        void SetDrawColor(const SDL_Color& color);
        SDL_Renderer* GetSDLRenderer() const;

        private:
        SDL_RendererFlags m_rendererFlags{SDL_RENDERER_ACCELERATED};
        SDL_Renderer* m_sdlRenderer{nullptr};
        
    };
    
} // namespace nxsg



#endif // NXSG_RENDERER_H