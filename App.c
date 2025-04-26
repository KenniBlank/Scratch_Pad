#include <SDL2/SDL_config_unix.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "point.h"
#include "helper.h"

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)
#define __DEBUG__(msg, ...)\
        printf("Debug at line %d: " msg "\n", __LINE__, ##__VA_ARGS__);\
        fflush(stdout);

// TODO: Render at this but show in whatever resolution user uses.
#define VIRTUAL_WIDTH 1900
#define VIRTUAL_HEIGHT 1080

#ifdef DEBUG
    #define SAVE_LOCATION "Images/"
#else
    #define SAVE_LOCATION "Pictures/"
#endif

#define BUFFER_SIZE (uint8_t)100

#define swap(a, b) \
    do { \
        typeof(*a) temp = *a; \
        *a = *b; \
        *b = temp; \
    } while (0)

enum Mode: uint8_t {
        MODE_NONE,
        MODE_TYPING,
        MODE_DRAWING,
        MODE_PAN,
        MODE_ERASOR,
};

typedef struct {
        LinesArray lines;
        Pan pan;
        enum Mode current_mode;
} TotalData;

// Global Variables:
SDL_Cursor* arrowCursor;
SDL_Cursor* crosshairCursor;
SDL_Cursor* sizeallCursor;
SDL_Cursor* handCursor;

void handle_cursor_change(enum Mode current_mode) {
        switch (current_mode) {
                case MODE_NONE:
                case MODE_TYPING:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
                        break;
                case MODE_DRAWING:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
                        break;
                case MODE_PAN:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
                        break;
                case MODE_ERASOR:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
                        break;
        }
}

int main(void) {
        int window_width = 900, window_height = 500;

        uint8_t LINE_THICKNESS = 3;

        SDL_Init(SDL_INIT_EVERYTHING);

        SDL_Window* window = SDL_CreateWindow(
                "Scratch Pad",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                window_width, window_height,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE
        );

        #ifdef DEBUG
                SDL_Renderer* renderer = SDL_CreateRenderer(
                        window,
                        -1,
                        SDL_RENDERER_SOFTWARE
                );
        #else
                SDL_Renderer* renderer = SDL_CreateRenderer(
                        window,
                        -1,
                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
                );
        #endif

        bool app_is_running = true;
        // TODO: Create Loading Screen...

        arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        crosshairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        sizeallCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

        SDL_Color
                bg_color = {0, 0, 0, 255}, // Background Color
                draw_color = {255, 255, 255, 255},
                UI_bg_color = {100, 100, 100, 255};

        TotalData DATA_INIT_VALUE = {
                .lines = {
                        .points = NULL,
                        .pointCount = 0,
                        .pointCapacity = 0,
                },
                .current_mode = MODE_NONE,
                .pan.x = 0,
                .pan.y = 0,
        };


        TotalData Data[BUFFER_SIZE];
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
                Data[i] = DATA_INIT_VALUE;
        }

        // This is where all of lines are drawn so that no need to rerender eveything
        SDL_Texture *drawLayer = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                (window_width << 2) / 5, // - window_width / 5 // TODO: 1/5 for UI Elements if certain window_size
                window_height
        );

        SDL_Texture *UILayer = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                (window_width) / 5, // - window_width / 5 // TODO: 1/5 for UI Elements if certain window_size
                window_height
        );
        SDL_SetRenderTarget(renderer, UILayer);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // light grey
        SDL_RenderClear(renderer);

        bool rerender = false;

        Data[0].current_mode = MODE_DRAWING;

        SDL_Event event;
        enum Mode current_mode;
        uint16_t line_start_index;

        while (app_is_running) {
                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT:
                                        app_is_running = false;
                                        break;

                                case SDL_WINDOWEVENT:
                                        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                                SDL_GetWindowSize(window, &window_width, &window_height);
                                                SDL_Texture* old = drawLayer;

                                                drawLayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);

                                                // Copy content from old layer to new one
                                                SDL_SetRenderTarget(renderer, drawLayer);
                                                SDL_RenderCopy(renderer, old, NULL, NULL);
                                                SDL_SetRenderTarget(renderer, NULL);

                                                SDL_DestroyTexture(old);
                                        }
                                        rerender = true;
                                        break;

                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                        app_is_running = false;
                                                        break;
                                                case SDLK_n:
                                                        Data[0].current_mode = MODE_NONE;
                                                        break;
                                                case SDLK_t:
                                                        Data[0].current_mode = MODE_TYPING;
                                                        break;
                                                case SDLK_d:
                                                        Data[0].current_mode = MODE_DRAWING;
                                                        break;
                                                case SDLK_p:
                                                        Data[0].current_mode = MODE_PAN;
                                                        break;
                                                case SDLK_e:
                                                        Data[0].current_mode = MODE_ERASOR;
                                                        break;
                                                case SDLK_s:
                                                        SaveRendererAsImage(renderer, "__image__", SAVE_LOCATION);
                                                        break;
                                        }
                                        break;
                                case SDL_MOUSEBUTTONDOWN:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        current_mode = Data->current_mode;
                                                        switch (current_mode) {
                                                                case MODE_DRAWING:
                                                                        addPoint(&Data[0].lines, (float) (event.button.x  - Data[0].pan.x), (float) (event.button.y  - Data[0].pan.y), LINE_THICKNESS, true);
                                                                        line_start_index = Data[0].lines.pointCount - 1;
                                                                        break;
                                                                case MODE_ERASOR:
                                                                        break;
                                                                default:
                                                                        rerender = true;
                                                                        break;
                                                        }
                                                        break;
                                        }
                                        break;
                                case SDL_MOUSEBUTTONUP:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        switch (current_mode) {
                                                                case MODE_DRAWING: {
                                                                                addPoint(&Data[0].lines, (float) (event.button.x  - Data[0].pan.x), (float) (event.button.y  - Data[0].pan.y), LINE_THICKNESS, true);
                                                                                OptimizeLine(&Data[0].lines, line_start_index, Data[0].lines.pointCount - 1);
                                                                                addPoint(&Data[0].lines, (float) (event.button.x  - Data[0].pan.x), (float) (event.button.y  - Data[0].pan.y), LINE_THICKNESS, false);
                                                                                rerender = true;
                                                                                break;
                                                                        }
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
                                                case MODE_PAN:
                                                        PanPoints(&Data[0].pan, (double) (event.motion.xrel), (double) (event.motion.yrel));
                                                        rerender = true;
                                                        break;
                                                case MODE_DRAWING:
                                                        addPoint(&Data[0].lines, (float) (event.button.x  - Data[0].pan.x), (float) (event.button.y  - Data[0].pan.y), LINE_THICKNESS, true);
                                                        break;
                                                default: break;
                                        }
                                        break;
                        }
                }

                handle_cursor_change(Data[0].current_mode);

                SDL_SetRenderTarget(renderer, drawLayer);
                if (rerender) {
                        // Clear Screen
                        SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                        SDL_RenderClear(renderer);

                        ReRenderLines(renderer, &Data[0].lines, Data[0].pan, draw_color);
                        rerender = false;
                } else {
                        RenderLines(renderer, &Data[0].lines, Data[0].pan, draw_color);
                }

                SDL_SetRenderTarget(renderer, NULL);

                // Clear Screen
                SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                SDL_RenderClear(renderer);

                // Copy static things back to renderer and render it
                SDL_RenderCopy(renderer, drawLayer, NULL, NULL);

                SDL_RenderPresent(renderer);

                #ifdef DEBUG
                        // print_live_usage();
                        SDL_Delay(16); // ~60 FPS
                #endif
        }

        for (size_t i = 0; i < BUFFER_SIZE; i++) {
                if (Data[i].lines.points != NULL) {
                        free(Data[i].lines.points);
                }
        }

        SDL_FreeCursor(arrowCursor);
        SDL_FreeCursor(crosshairCursor);
        SDL_FreeCursor(sizeallCursor);
        SDL_FreeCursor(handCursor);

        SDL_DestroyTexture(drawLayer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}
