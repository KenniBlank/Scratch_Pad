#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "point.h"
#include "helper.h"

#define WINDOW_WIDTH (uint16_t) 900
#define WINDOW_HEIGHT (uint16_t) 500
#define LINE_THICKNESS (uint8_t) 3

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)

enum Mode: uint8_t {
        MODE_NONE,
        MODE_DRAWING,
        MODE_PAN,
        MODE_ERASOR,
        MODE_TYPING
};

typedef struct {
        LinesArray lines;
        enum Mode current_mode;
} TotalData;

void handle_events(SDL_Renderer* renderer, TotalData *Data, bool* app_running, bool* update_renderer) {
        static SDL_Event event;
        static enum Mode current_mode;

        while (SDL_PollEvent(&event)) {
                switch (event.type) {
                        case SDL_QUIT:
                                *app_running = false;
                                break;

                        case SDL_KEYDOWN:
                                switch (event.key.keysym.sym) {
                                        case SDLK_ESCAPE:
                                                *app_running = false;
                                                break;
                                        case SDLK_s:
                                                SaveRendererAsImage(renderer, "__image__", "images/");
                                                break;
                                }
                                break;
                        case SDL_MOUSEBUTTONDOWN:
                                switch (event.button.button) {
                                        case SDL_BUTTON_LEFT:
                                                current_mode = Data->current_mode;
                                                *update_renderer = true;
                                                switch (current_mode) {
                                                        case MODE_DRAWING:
                                                                addPoint(&Data->lines, event.button.x, event.button.y, LINE_THICKNESS, true);
                                                                break;
                                                        default:
                                                                break;
                                                }
                                                break;
                                }
                                break;
                        case SDL_MOUSEBUTTONUP:
                                switch (event.button.button) {
                                        case SDL_BUTTON_LEFT:
                                                *update_renderer = true;
                                                switch (current_mode) {
                                                        case MODE_DRAWING:
                                                                addPoint(&Data->lines, event.button.x, event.button.y, LINE_THICKNESS, false);
                                                                break;
                                                        default:
                                                                break;
                                                }
                                                current_mode = MODE_NONE;
                                                break;
                                }
                                break;
                        case SDL_MOUSEMOTION:
                                switch (current_mode) {
                                        case MODE_NONE:
                                                break;
                                        case MODE_DRAWING:
                                                *update_renderer = true;
                                                addPoint(&Data->lines, event.motion.x, event.motion.y, LINE_THICKNESS, true);
                                                break;
                                        default: break;

                                }
                                break;
                }
        }
}

int main(void) {
        SDL_Init(SDL_INIT_EVERYTHING);

        SDL_Window* window = SDL_CreateWindow(
                "Scratch Pad",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH, WINDOW_HEIGHT,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
        );

        SDL_Renderer* renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        bool app_is_running = true;
        bool update_renderer = true;

        SDL_Color
                bg_color = {15, 20, 25, 255}, // Background Color
                white_color = {255, 255, 255, 255};

        TotalData DATA_INIT_VALUE = {
                .lines = {
                        .points = NULL,
                        .pointCount = 0,
                        .pointCapacity = 0,
                        .color = white_color
                },
                .current_mode = MODE_NONE
        };

        #define BUFFER_SIZE (uint8_t)100

        TotalData Data[BUFFER_SIZE];
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
                Data[i] = DATA_INIT_VALUE;
        }

        Data[0].current_mode = MODE_DRAWING;
        while (app_is_running) {
                handle_events(renderer, &Data[0], &app_is_running, &update_renderer);
                if (update_renderer) {
                        SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                        SDL_RenderClear(renderer);

                        ReRenderLines(renderer, &Data[0].lines);

                        update_renderer = false;
                        print_live_usage();

                        SDL_RenderPresent(renderer);
                }

        }

        for (size_t i = 0; i < BUFFER_SIZE; i++) {
                if (Data[i].lines.points != NULL) {
                        free(Data[i].lines.points);
                }
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}
