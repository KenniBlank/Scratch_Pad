#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600

double func(double x) {
    return atan(x);  // Change this for other functions
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Function Visualizer with Pan and Zoom", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    int offsetX = 0, offsetY = 0;
    double zoom = 50.0; // Initial zoom level
    int quit = 0;
    SDL_Event e;

    while (!quit) {
        // Event handling
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = 1;
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:  offsetX -= 10; break;
                    case SDLK_RIGHT: offsetX += 10; break;
                    case SDLK_UP:    offsetY -= 10; break;
                    case SDLK_DOWN:  offsetY += 10; break;
                    case SDLK_PLUS:  // Zoom in
                        zoom *= 1.1;
                        break;
                    case SDLK_MINUS: // Zoom out
                        zoom /= 1.1;
                        break;
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);

        // Draw axes
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderDrawLine(ren, 0, HEIGHT / 2 + offsetY, WIDTH, HEIGHT / 2 + offsetY); // X-axis
        SDL_RenderDrawLine(ren, WIDTH / 2 + offsetX, 0, WIDTH / 2 + offsetX, HEIGHT);  // Y-axis

        // Draw function
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        for (int x = 0; x < WIDTH; ++x) {
            double fx = (x - WIDTH / 2.0 - offsetX) / zoom;
            double fy = func(fx);
            int y = HEIGHT / 2 - (int)(fy * zoom) + offsetY;
            SDL_RenderDrawPoint(ren, x, y);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
