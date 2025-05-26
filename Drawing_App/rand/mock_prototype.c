#include <SDL2/SDL.h>
#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)

int window_width = 800, window_height = 600;

int main(void) {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Window* window = SDL_CreateWindow("Mock App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_SHOWN);
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_Event event;
        SDL_bool app_running = SDL_TRUE;

        SDL_Color bg_color = {
                .r = 255,
                .g = 255,
                .b = 255,
                .a = 255
        };
        SDL_Color fg_color = {
                .r = 0,
                .g = 0,
                .b = 0,
                .a = 255
        };
        Uint32 delay_ms = 1000 / 60; // 60 FPS

        while (app_running) {
                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT:
                                        app_running = SDL_FALSE;
                                        break;
                        }
                }

                SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                SDL_RenderClear(renderer);

                SDL_SetRenderDrawColor(renderer, unpack_color(fg_color));
                SDL_RenderPresent(renderer);

                SDL_Delay(delay_ms);
        }

        return 0;
}
