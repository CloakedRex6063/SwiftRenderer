#include "Swift.h"
#include "SDL3/SDL.h"
int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    auto* window = SDL_CreateWindow("Example", 1280, 720, SDL_WINDOW_RESIZABLE);

    SwiftRenderer swiftRenderer;
    swiftRenderer.Init();
    
    bool isRunning = true;
    while (isRunning)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                isRunning = false;
                break;
            default:
                break;
        }

        swiftRenderer.Render();
    }

    swiftRenderer.Shutdown();
    
    SDL_DestroyWindow(window);
    SDL_Quit();
}
