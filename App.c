#include <SDL2/SDL_config_unix.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
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

#include "point.h"
#include "helper.h"

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)
#define __DEBUG__(msg, ...)\
        printf("Debug at line %d: " msg "\n", __LINE__, ##__VA_ARGS__);\
        fflush(stdout);

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
        MODE_DRAWING,
        MODE_PAN,
        MODE_ERASOR,
        MODE_TYPING
};

typedef struct {
        LinesArray lines;
        Pan pan;
        enum Mode current_mode;
} TotalData;

double perpendicularDistance(Point pt, Point lineStart, Point lineEnd) {
        double dx = lineEnd.x - lineStart.x;
        double dy = lineEnd.y - lineStart.y;

        if (dx == 0 && dy == 0) {
                dx = pt.x - lineStart.x;
                dy = pt.y - lineStart.y;
                return sqrt(dx * dx + dy * dy);
        }

        double num = fabs(dy * pt.x - dx * pt.y + lineEnd.x * lineStart.y - lineEnd.y * lineStart.x);
        double den = sqrt(dx * dx + dy * dy);
        return num / den;
}

void RDP(Point* points, uint16_t line_start_index, uint16_t line_end_index, double epsilon, bool* keep_indexes) {
        // Base case
        if (line_end_index <= line_start_index + 1) {
                keep_indexes[line_start_index - line_start_index] = true;
                keep_indexes[line_end_index - line_start_index] = true;
                return;
        }

        double maxDistance = 0.0;
        int index = -1;

        for (int i = line_start_index + 1; i < line_end_index; i++) { // No need to check distance for end and start points :)
                double dist = perpendicularDistance(points[i], points[line_start_index], points[line_end_index]);
                if (dist > maxDistance) {
                        maxDistance = dist;
                        index = i;
                }
        }

        if (maxDistance >= epsilon && index != -1) {
                keep_indexes[index - line_start_index] = true;
                RDP(points, line_start_index, index, epsilon, keep_indexes);
                RDP(points, index, line_end_index, epsilon, keep_indexes);
        } else {
                // Endpoints should be kept
                keep_indexes[line_start_index - line_start_index] = true;
                keep_indexes[line_end_index - line_start_index] = true;
        }
}

void OptimizeLine(LinesArray* PA, uint16_t line_start_index, uint16_t line_end_index, double epsilon) {
        int total_range_of_points = PA->pointCount - line_start_index + 1;
        bool *keep_indexes = calloc(total_range_of_points, sizeof(bool));

        RDP(PA->points, line_start_index, line_end_index, epsilon, keep_indexes);
        for (int i = 0; i < total_range_of_points; i++) {
                printf("%d\n", keep_indexes[i]);
        }

        free(keep_indexes);
}

void handle_events(
        SDL_Window* window,
        SDL_Renderer* renderer,
        TotalData *Data,
        SDL_Texture **staticLayer,
        int* window_width,
        int* window_height,
        bool* app_running,
        bool* update_renderer,
        bool* rerender,
        const uint8_t LINE_THICKNESS
) {
        static SDL_Event event;
        static enum Mode current_mode;
        static uint16_t line_start_index;

        while (SDL_PollEvent(&event)) {
                switch (event.type) {
                        case SDL_QUIT:
                                *app_running = false;
                                break;

                        case SDL_WINDOWEVENT:
                                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                        SDL_GetWindowSize(window, window_width, window_height);
                                        SDL_Texture* old = *staticLayer;

                                        *staticLayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, *window_width, *window_height);

                                        // Copy content from old to new
                                        SDL_SetRenderTarget(renderer, *staticLayer);
                                        SDL_RenderCopy(renderer, old, NULL, NULL);
                                        SDL_SetRenderTarget(renderer, NULL);

                                        SDL_DestroyTexture(old);
                                }
                                *update_renderer = true;
                                break;

                        case SDL_KEYDOWN:
                                switch (event.key.keysym.sym) {
                                        case SDLK_ESCAPE:
                                                *app_running = false;
                                                break;
                                        case SDLK_n:
                                                Data->current_mode = MODE_NONE;
                                                break;
                                        case SDLK_d:
                                                Data->current_mode = MODE_DRAWING;
                                                break;
                                        case SDLK_p:
                                                Data->current_mode = MODE_PAN;
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
                                                *update_renderer = true;
                                                switch (current_mode) {
                                                        case MODE_DRAWING:
                                                                addPoint(&Data->lines, event.button.x  - Data->pan.x, event.button.y  - Data->pan.y, LINE_THICKNESS, true);
                                                                line_start_index = Data->lines.pointCount - 1;
                                                                break;
                                                        default:
                                                                *rerender = true;
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
                                                        case MODE_DRAWING: {
                                                                        addPoint(&Data->lines, event.button.x - Data->pan.x, event.button.y - Data->pan.y, LINE_THICKNESS, false);

                                                                        double epsilon = 5.0f;
                                                                        OptimizeLine(&Data->lines, line_start_index, Data->lines.pointCount - 1, epsilon);
                                                                        *rerender = true;
                                                                        break;
                                                                }
                                                        default:
                                                                *rerender = true;
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
                                                PanPoints(&Data->pan, event.motion.xrel, event.motion.yrel);
                                                *update_renderer = true;
                                                *rerender = true;
                                                break;
                                        case MODE_DRAWING:
                                                addPoint(&Data->lines, event.motion.x - Data->pan.x, event.motion.y - Data->pan.y, LINE_THICKNESS, true);

                                                *update_renderer = true;
                                                break;
                                        default: break;
                                }
                                break;
                }
        }
}

void handle_cursor_change(enum Mode current_mode) {
        switch (current_mode) {
                case MODE_DRAWING:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
                        break;
                case MODE_PAN:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
                        break;
                case MODE_NONE:
                        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
                        break;
                default:
                        break;
        }
}

int main(void) {
        int WINDOW_WIDTH = 900, WINDOW_HEIGHT = 500;
        uint8_t LINE_THINKNESS = 3;

        SDL_Init(SDL_INIT_EVERYTHING);

        SDL_Window* window = SDL_CreateWindow(
                "Scratch Pad",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH, WINDOW_HEIGHT,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP
        );

        #ifdef DEBUG
                SDL_Renderer* renderer = SDL_CreateRenderer(
                        window,
                        -1,
                        SDL_RENDERER_ACCELERATED
                );
        #else
                SDL_Renderer* renderer = SDL_CreateRenderer(
                        window,
                        -1,
                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
                );
        #endif

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        bool app_is_running = true;
        bool update_renderer = true;

        SDL_Color
                bg_color = {0, 0, 0, 255}, // Background Color
                draw_color = {255, 255, 255, 255};

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

        // Static Background Texture:
        //      This is where all of lines are drawn so that no need to rerender eveything
        SDL_Texture *staticLayer = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                WINDOW_WIDTH, WINDOW_HEIGHT
        );

        bool rerender = false;

        Data[0].current_mode = MODE_DRAWING;

        while (app_is_running) {
                handle_events(
                        window,
                        renderer,
                        &Data[0],
                        &staticLayer,
                        &WINDOW_WIDTH,
                        &WINDOW_HEIGHT,
                        &app_is_running,
                        &update_renderer,
                        &rerender,
                        LINE_THINKNESS
                );

                handle_cursor_change(Data[0].current_mode);

                if (update_renderer) {
                        update_renderer = false;

                        SDL_SetRenderTarget(renderer, staticLayer);
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
                        SDL_RenderCopy(renderer, staticLayer, NULL, NULL);
                        SDL_RenderPresent(renderer);

                        #ifdef DEBUG
                                // print_live_usage();
                        #endif
                }
                #ifdef DEBUG
                        SDL_Delay(16); // ~60 FPS
                #endif
        }

        for (size_t i = 0; i < BUFFER_SIZE; i++) {
                if (Data[i].lines.points != NULL) {
                        free(Data[i].lines.points);
                }
        }

        SDL_DestroyTexture(staticLayer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}
