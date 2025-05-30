#include <SDL2/SDL_config_unix.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "point.h"
#include "helper.h"

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)

#ifdef DEBUG
    #define SAVE_LOCATION "Images/"
#else
    #define SAVE_LOCATION "Pictures/"
#endif

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

typedef struct {
        SDL_Texture **data;
        size_t capacity;
        size_t count;
} TextureArray;


// Global Variables:
SDL_Cursor* arrowCursor;
SDL_Cursor* crosshairCursor;
SDL_Cursor* panCursor;
SDL_Cursor* erasorCursor;

void handle_cursor_change(enum Mode current_mode) {
        switch (current_mode) {
                case MODE_NONE: case MODE_TYPING: SDL_SetCursor(arrowCursor); break;
                case MODE_DRAWING: SDL_SetCursor(crosshairCursor); break;
                case MODE_PAN: SDL_SetCursor(panCursor); break;
                case MODE_ERASOR: SDL_SetCursor(erasorCursor); break;
        }
}

void pushTexture(TextureArray *arr, SDL_Texture *tex) {
        if (arr->count >= arr->capacity) {
                arr->capacity *= 2;
                arr->data = realloc(arr->data, arr->capacity * sizeof(SDL_Texture *));
                if (!arr->data) {
                perror("realloc failed");
                exit(1);
                }
        }
        arr->data[arr->count] = tex;
        arr->count++;
}

int main(void) {
        int window_width = 900, window_height = 600;

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
                        // Note: Don't use SDL_RENDERER_ACCELERATED: Calculation from CPU makes lines much better
                        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE // | SDL_RENDERER_ACCELERATED
                );
        #endif

        bool app_is_running = true;
        // TODO: Create Loading Screen...

        arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        crosshairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        panCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        erasorCursor = createCursorFromPNG("Icons/eraser_cursor.png", 16, 16);

        SDL_Color
                ui_bg_color = {35, 35, 41, 255},
                bg_color = {0, 0, 0, 255},
                draw_color = {255, 255, 255, 255};

        TotalData Data = {
                .lines = {
                        .points = NULL,
                        .pointCount = 0,
                        .pointCapacity = 0,
                },
                .current_mode = MODE_NONE,
                .pan.x = 0,
                .pan.y = 0,
        };

        // This is where all of lines are drawn
        TextureArray drawLayers = {
                .data = malloc(2 * sizeof(SDL_Texture *)),
                .capacity = 2,
                .count = 0
        };

        drawLayers.data[0] = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                window_width,
                window_height
        );
        drawLayers.data[1] = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                window_width,
                window_height
        );
        drawLayers.count += 2;

        size_t current_drawLayers_index = drawLayers.count - 1;
        SDL_Texture *drawLayer = drawLayers.data[current_drawLayers_index];

        // Icons
        SDL_Rect toolLayerRect = {
                .w = 200,
                .h = 70,
                .y = 20,
                .x = (window_width - toolLayerRect.w) >> 1,
        };

        // STATIC:
        SDL_Texture *ToolsLayer = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                toolLayerRect.w,
                toolLayerRect.h
        );

        SDL_Texture* penIcon = LoadImageAsTexture("Icons/pen_tool.png", renderer);
        SDL_Texture* eraserIcon = LoadImageAsTexture("Icons/eraser.png", renderer);
        SDL_Texture* panIcon = LoadImageAsTexture("Icons/pan_tool.png", renderer);

        SDL_SetRenderTarget(renderer, ToolsLayer);
        SDL_SetRenderDrawColor(renderer, unpack_color(ui_bg_color));
        SDL_RenderClear(renderer);

        SDL_Rect penRect = { 15, 10, 50, 50 };
        SDL_Rect eraserRect = { 75, 10, 50, 50 };
        SDL_Rect panRect = { 135, 10, 50, 50 };

        SDL_RenderCopy(renderer, penIcon, NULL, &penRect);
        SDL_RenderCopy(renderer, eraserIcon, NULL, &eraserRect);
        SDL_RenderCopy(renderer, panIcon, NULL, &panRect);
        SDL_SetRenderDrawColor(renderer, unpack_color(draw_color));
        SDL_RenderDrawRect(renderer, &penRect);
        SDL_RenderDrawRect(renderer, &eraserRect);
        SDL_RenderDrawRect(renderer, &panRect);
        SDL_SetRenderTarget(renderer, NULL);

        bool rerender = true;

        Data.current_mode = MODE_DRAWING;

        SDL_Event event;
        enum Mode current_mode;
        uint16_t line_start_index;
        bool newLineAdded = false;

        while (app_is_running) {
                drawLayer = drawLayers.data[current_drawLayers_index];

                handle_cursor_change(Data.current_mode);
                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT: app_is_running = false; break;
                                case SDL_WINDOWEVENT:
                                        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                                SDL_GetWindowSize(window, &window_width, &window_height);
                                                set_window_dimensions(window_width, window_height);
                                                SDL_Texture* old = drawLayer;

                                                drawLayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
                                                SDL_SetTextureBlendMode(drawLayer, SDL_BLENDMODE_BLEND);

                                                // Copy content from old layer to new one, and destroy previous one
                                                SDL_SetRenderTarget(renderer, drawLayer);
                                                SDL_RenderCopy(renderer, old, NULL, NULL);
                                                SDL_SetRenderTarget(renderer, NULL);
                                                SDL_DestroyTexture(old);

                                                // Other:
                                                toolLayerRect.x = (window_width - toolLayerRect.w) >> 1;
                                        }
                                        break;
                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym) {
                                                case SDLK_ESCAPE: app_is_running = false; break;
                                                case SDLK_p: Data.current_mode = MODE_PAN; break;
                                                case SDLK_n: Data.current_mode = MODE_NONE; break;
                                                case SDLK_e: Data.current_mode = MODE_ERASOR; break;
                                                case SDLK_t: Data.current_mode = MODE_TYPING; break;
                                                case SDLK_d: Data.current_mode = MODE_DRAWING; break;
                                        }

                                        if (event.key.keysym.mod & KMOD_LCTRL) {
                                                switch (event.key.keysym.sym) {
                                                        case SDLK_s:
                                                                SaveRendererAsImage(renderer, "__image__", SAVE_LOCATION);
                                                                break;
                                                        case SDLK_z:
                                                                // Undo
                                                                if (current_drawLayers_index > 0) {
                                                                        current_drawLayers_index--;
                                                                } else {
                                                                        current_drawLayers_index = 0;
                                                                }
                                                                break;
                                                        case SDLK_y:
                                                                // Redo
                                                                if (current_drawLayers_index + 1 >= drawLayers.count) {
                                                                        current_drawLayers_index = drawLayers.count - 1;
                                                                } else {
                                                                        current_drawLayers_index++;
                                                                }
                                                                break;
                                                }
                                        }
                                        break;
                                case SDL_MOUSEBUTTONDOWN:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        if (CollisionDetection(event.button.x, event.button.y, 1, 1, toolLayerRect.x, toolLayerRect.y, toolLayerRect.w, toolLayerRect.h)) {
                                                                if (CollisionDetection(event.button.x, event.button.y, 1, 1, toolLayerRect.x + eraserRect.x, toolLayerRect.y + eraserRect.y, eraserRect.w, eraserRect.h)) {
                                                                        Data.current_mode = MODE_ERASOR;
                                                                        break;
                                                                } else if (CollisionDetection(event.button.x, event.button.y, 1, 1, toolLayerRect.x + penRect.x, toolLayerRect.y + penRect.y, penRect.w, penRect.h)) {
                                                                        Data.current_mode = MODE_DRAWING;
                                                                        break;
                                                                } else if (CollisionDetection(event.button.x, event.button.y, 1, 1, toolLayerRect.x + panRect.x, toolLayerRect.y + panRect.y, panRect.w, panRect.h)) {
                                                                        Data.current_mode = MODE_PAN;
                                                                        break;
                                                                }
                                                        }

                                                        current_mode = Data.current_mode;
                                                        switch (current_mode) {
                                                                case MODE_DRAWING:
                                                                        addPoint(&Data.lines, (float) (event.button.x  - Data.pan.x), (float) (event.button.y  - Data.pan.y), LINE_THICKNESS, true);
                                                                        line_start_index = Data.lines.pointCount - 1;
                                                                        break;
                                                                default: break;
                                                        }
                                                        break;
                                        }
                                        break;
                                case SDL_MOUSEBUTTONUP:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        switch (current_mode) {
                                                                case MODE_DRAWING: {
                                                                        addPoint(&Data.lines, (float) (event.button.x  - Data.pan.x), (float) (event.button.y  - Data.pan.y), LINE_THICKNESS, false);
                                                                        newLineAdded = true;
                                                                        break;
                                                                }
                                                                default: break;
                                                        }
                                                        current_mode = MODE_NONE;
                                                        break;
                                        }
                                        break;
                                case SDL_MOUSEMOTION:
                                        switch (current_mode) {
                                                case MODE_NONE: break;
                                                case MODE_PAN:
                                                        PanPoints(&Data.pan, (double) (event.motion.xrel), (double) (event.motion.yrel));
                                                        rerender = true;
                                                        break;
                                                case MODE_DRAWING:
                                                        addPoint(&Data.lines, (float) (event.button.x  - Data.pan.x), (float) (event.button.y  - Data.pan.y), LINE_THICKNESS, true);
                                                        break;
                                                default: break;
                                        }
                                        break;
                        }
                }

                if (rerender) {
                        SDL_SetRenderTarget(renderer, drawLayer);

                        SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                        SDL_RenderClear(renderer);

                        ReRenderLines(renderer, &Data.lines, Data.pan, draw_color);
                        SDL_SetRenderTarget(renderer, NULL);
                        rerender = false;
                }

                // If new line ended, save for future undo/redo action
                if (newLineAdded) {
                        // Blueprint IG:
                        // 1. Copy all previous strokes from drawLayer to newLayer
                        // 2. Render new strokes to new layer
                        // 3. Save new layer as drawLayer

                        OptimizeLine(&Data.lines, line_start_index, Data.lines.pointCount - 1);
                        SDL_Texture *newLayer = SDL_CreateTexture(
                                renderer,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_TARGET,
                                window_width,
                                window_height
                        );
                        if (!newLayer) {
                                fprintf(stdout, "Failed to create texture: %s\n", SDL_GetError());
                                break;
                        }

                        SDL_SetRenderTarget(renderer, newLayer);
                        SDL_RenderCopy(renderer, drawLayers.data[current_drawLayers_index], NULL, NULL);

                        __RenderLines__(renderer, &Data.lines, Data.pan, line_start_index, Data.lines.pointCount - 1, draw_color);

                        if (current_drawLayers_index < drawLayers.count - 1) {
                                // Delete all texture after that index:
                                for (size_t i = current_drawLayers_index + 1; i < drawLayers.count; i++) {
                                        if (drawLayers.data[i]) {
                                                SDL_DestroyTexture(drawLayers.data[i]);
                                        }
                                }
                                drawLayers.count = current_drawLayers_index + 1;
                        }

                        SDL_SetRenderTarget(renderer, NULL);
                        // Save newTexture:
                        pushTexture(&drawLayers, newLayer);
                        current_drawLayers_index = drawLayers.count - 1;
                        newLineAdded = false;
                }

                // Copy DrawLayers's content to renderer
                SDL_RenderCopy(renderer, drawLayer, NULL, NULL);

                // Tools:
                SDL_RenderCopy(renderer, ToolsLayer, NULL, &toolLayerRect);

                // New line provided by usr
                RenderLine(renderer, &Data.lines, Data.pan, Data.lines.rendered_till, Data.lines.pointCount, (SDL_Color) { .r = 0, .g = 100, .b = 100, .a = 150 });

                SDL_RenderPresent(renderer);

                #ifdef DEBUG
                        // print_live_usage();
                        SDL_Delay(22); // ~45 FPS
                #endif
        }

        if (Data.lines.points != NULL) {
                free(Data.lines.points);
        }

        SDL_FreeCursor(arrowCursor);
        SDL_FreeCursor(crosshairCursor);
        SDL_FreeCursor(panCursor);
        SDL_FreeCursor(erasorCursor);

        for (size_t i = 0; i < drawLayers.count; i++) {
                if (drawLayers.data[i]) {
                        SDL_DestroyTexture(drawLayers.data[i]);
                }
        }
        free(drawLayers.data);

        SDL_DestroyTexture(penIcon);
        SDL_DestroyTexture(panIcon);
        SDL_DestroyTexture(eraserIcon);
        SDL_DestroyTexture(ToolsLayer);

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}
