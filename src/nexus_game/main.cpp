#include "nexus.h"
#include <SDL.h>

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Window",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          680, 480,
                                          0);

    if (!window)
    {
        return -1;
    }

    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);

    if (!windowSurface)
    {
        return -1;
    }

    bool isRunning = true;
    while (isRunning)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                isRunning = false;
                break;
            }
            SDL_UpdateWindowSurface(window);
        }
    }

    return 0;
}