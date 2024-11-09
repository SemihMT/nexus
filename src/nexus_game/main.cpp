#include "nexus.h"
#include <SDL.h>
#include <iostream>
#include "nxsg_window.h"
#include "nxsg_renderer.h"

bool HandleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            return true;
            break;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }

    nxsg::window window{};
    nxsg::renderer renderer{window};

    bool shouldClose{false};
    while (!shouldClose)
    {
        // Process input
        shouldClose = HandleEvents();   
        
        // Update the game here:
        // TODO: update function
        
        
        renderer.SetDrawColor({128,0, 0, 255});
        renderer.Clear();
        // Do rendering here

        renderer.Present();
    }

    SDL_Quit();
    return 0;
}
