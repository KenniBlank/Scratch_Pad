#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_POINTS 100000

typedef struct {
        float x, y;
} Point;

Point points[MAX_POINTS];
int point_count = 0;

int main() {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Window *win = SDL_CreateWindow("Vector Zoom", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

        int quit = 0, drawing = 0;
        float scale = 1.0f;
        SDL_FPoint offset = {0, 0};

        while (!quit) {
                SDL_Event e;
                while (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT)
                                quit = 1;
                        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                                quit = 1;
                        }
                        else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                                drawing = 1;
                        else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
                                drawing = 0;
                        else if (e.type == SDL_MOUSEWHEEL) {
                                float old_scale = scale;
                                scale *= (e.wheel.y > 0) ? 1.1f : 0.9f;

                                int mx, my;
                                SDL_GetMouseState(&mx, &my);
                                float cx = (mx - offset.x) / old_scale;
                                float cy = (my - offset.y) / old_scale;
                                offset.x = mx - cx * scale;
                                offset.y = my - cy * scale;
                        }
                }

                if (drawing && point_count < MAX_POINTS) {
                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        points[point_count].x = (mx - offset.x) / scale;
                        points[point_count].y = (my - offset.y) / scale;
                        point_count++;
                }

                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderClear(ren);

                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                for (int i = 1; i < point_count; ++i) {
                        SDL_RenderDrawLine(
                                ren,
                                (int)(points[i - 1].x * scale + offset.x),
                                (int)(points[i - 1].y * scale + offset.y),
                                (int)(points[i].x * scale + offset.x),
                                (int)(points[i].y * scale + offset.y)
                        );
                }

                SDL_RenderPresent(ren);
        }

        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
}
