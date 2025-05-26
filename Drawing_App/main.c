#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TITLE "Scratch Pad"
#define unpack_color(color) color.r, color.g, color.b, color.a
#define swap(a, b) \
    do { \
        typeof(*a) temp = *a; \
        *a = *b; \
        *b = temp; \
    } while (0)

const uint16_t CANVAS_WIDTH = 5000;
const uint16_t CANVAS_HEIGHT = 8000;
const uint8_t LINE_THICKNESS = 1; // TODO: Thickness Logic?

enum Mode: uint8_t {
        MODE_NONE,
        MODE_PEN,
        MODE_PAN,
        MODE_ERASER,
        MODE_TYPING
};

typedef struct Flags{
        bool app_running;
        bool re_render;
        bool show_grid;
        bool debug;
} Flags;

typedef struct {
        int Window_Width, Window_Height;
        int grid_size;
} WindowData;

// Global Variables: All
float g_scale = 1.0f;
float g_min_scale = 0.1f;

SDL_FPoint g_pan = {0};
enum Mode g_usr_selected_mode = MODE_NONE;

SDL_Color g_bg_color = {
        .r = 25,
        .g = 25,
        .b = 25,
        .a = 255
};
SDL_Color g_draw_color = {
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 255,
};
SDL_Color g_grid_color = {
        .r = 200,
        .g = 200,
        .b = 200,
        .a = 255
};

SDL_Cursor* arrowCursor;
SDL_Cursor* crosshairCursor;
SDL_Cursor* panCursor;
SDL_Cursor* erasorCursor;

// Functions:
void HandleCursorChange();
void DrawGrid(SDL_Renderer* renderer, WindowData win_data);
SDL_Cursor* createCursorFromPNG(const char* filename, uint8_t width, uint8_t height);

int main(void) {
        WindowData win_data = {
                .Window_Height = 700,
                .Window_Width = 900,
                .grid_size = 50
        };

        setenv("SDL_VIDEODRIVER", "wayland", 1);
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, "/dev/input/event*");

        SDL_Init(SDL_INIT_EVERYTHING);

        SDL_Window *window = SDL_CreateWindow(
                TITLE,
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                win_data.Window_Width, win_data.Window_Height,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
        );
        SDL_Renderer *renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
        );
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_Delay(1000); // 1s for Hyprland to register window

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "hyprctl keyword windowrulev2 'opacity 0.75, title:^(%s)$'", TITLE);
        system(cmd);

        // Variables
        arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        crosshairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        panCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        erasorCursor = createCursorFromPNG("App_Depencencies/icons/eraser_cursor.png", 24, 24);

        g_pan = (SDL_FPoint) {
                .x = ((float) CANVAS_WIDTH - win_data.Window_Width) / 2.0f,
                .y = ((float) CANVAS_HEIGHT - win_data.Window_Height) / 2.0f
        };

        SDL_Event event;
        Flags APP_FLAGS = {
                .app_running = true,
                .re_render = true,
                .debug = true,
                .show_grid = true,
        };
        enum Mode current_mode = MODE_NONE;

        while (APP_FLAGS.app_running) {
                HandleCursorChange();
                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT: APP_FLAGS.app_running = false; break;

                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                        APP_FLAGS.app_running = false;
                                                        break;

                                                case SDLK_d:
                                                        g_usr_selected_mode = MODE_PEN;
                                                        break;

                                                case SDLK_p:
                                                        g_usr_selected_mode = MODE_PAN;
                                                        break;

                                                case SDLK_t:
                                                        g_usr_selected_mode = MODE_TYPING;
                                                        break;

                                                case SDLK_e:
                                                        g_usr_selected_mode = MODE_ERASER;
                                                        break;
                                        }
                                        break;

                                case SDL_WINDOWEVENT:
                                        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                                SDL_GetWindowSize(window, &win_data.Window_Width, &win_data.Window_Height);
                                                APP_FLAGS.re_render = true;

                                                // Clamp Scale for new window size
                                                g_min_scale = fmaxf((float)win_data.Window_Width / CANVAS_WIDTH, (float)win_data.Window_Height / CANVAS_HEIGHT);
                                                g_scale = SDL_clamp(g_scale, g_min_scale * 1.01f, 2.0f);

                                                // Clamp g_pan to never go allow user to go beyond canvas limit
                                                g_pan.x = SDL_clamp(g_pan.x, 0, CANVAS_WIDTH * g_scale - win_data.Window_Width);
                                                g_pan.y = SDL_clamp(g_pan.y, 0, CANVAS_HEIGHT * g_scale - win_data.Window_Height);
                                        }
                                        break;

                                case SDL_MOUSEBUTTONDOWN:
                                        if (event.button.button == SDL_BUTTON_LEFT) {
                                                current_mode = g_usr_selected_mode;
                                        }

                                        switch (current_mode) {
                                                case MODE_PEN:
                                                        break;
                                                default: break;
                                        }
                                        break;

                                case SDL_MOUSEBUTTONUP:
                                        if (event.button.button == SDL_BUTTON_LEFT) {
                                                switch (current_mode) {
                                                        case MODE_PEN:
                                                                break;
                                                        default: break;

                                                }
                                                current_mode = MODE_NONE;
                                        }
                                        break;

                                case SDL_MOUSEMOTION:
                                        switch (current_mode) {
                                               case MODE_PAN:
                                                        // Clamp g_pan to never go allow user to go beyond canvas limit
                                                        g_pan.x = SDL_clamp(g_pan.x - event.motion.xrel, 0, CANVAS_WIDTH * g_scale - win_data.Window_Width);
                                                        g_pan.y = SDL_clamp(g_pan.y - event.motion.yrel, 0, CANVAS_HEIGHT * g_scale - win_data.Window_Height);
                                                        break;

                                                case MODE_PEN:
                                                        break;

                                               default: break;
                                        }

                                        if (current_mode != MODE_NONE) {
                                                APP_FLAGS.re_render = true;
                                        }
                                        break;

                                case SDL_MOUSEWHEEL: {
                                        float oldScale = g_scale;

                                        int mouseX, mouseY;
                                        SDL_GetMouseState(&mouseX, &mouseY);

                                        float worldX = (mouseX + g_pan.x) / g_scale;
                                        float worldY = (mouseY + g_pan.y) / g_scale;

                                        const float zoomFactor = 1.1f;
                                        if (event.wheel.y != 0) {
                                                g_scale *= powf(zoomFactor, event.wheel.y);
                                        }

                                        // Clamp Scale
                                        g_min_scale = fmaxf(
                                                (float)win_data.Window_Width / CANVAS_WIDTH,
                                                (float)win_data.Window_Height / CANVAS_HEIGHT
                                        );

                                        g_scale = SDL_clamp(g_scale, g_min_scale * 1.01f, 2.0f); // Max Zoom In: 200%, Min Zoom: dynamic->based on window size

                                        if (oldScale != g_scale) {
                                                g_pan.x = worldX * g_scale - mouseX;
                                                g_pan.y = worldY * g_scale - mouseY;
                                        }

                                        // Clamp g_pan to never go allow user to go beyond canvas limit
                                        g_pan.x = SDL_clamp(g_pan.x, 0, CANVAS_WIDTH * g_scale - win_data.Window_Width);
                                        g_pan.y = SDL_clamp(g_pan.y, 0, CANVAS_HEIGHT * g_scale - win_data.Window_Height);

                                        APP_FLAGS.re_render = true;
                                        break;
                                }
                        }
                }

                if (APP_FLAGS.re_render) {
                        APP_FLAGS.re_render = false;

                        // Clear Screen
                        SDL_SetRenderDrawColor(renderer, unpack_color(g_bg_color));
                        SDL_RenderClear(renderer);

                        if (APP_FLAGS.show_grid) DrawGrid(renderer, win_data);
                        if (APP_FLAGS.debug) {
                                // Sample rectangle (canvas-centered)
                                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                                SDL_Rect sample_rect = {
                                        .x = (int)(2400 * g_scale - g_pan.x),
                                        .y = (int)(2400 * g_scale - g_pan.y),
                                        .w = (int)(200 * g_scale),
                                        .h = (int)(200 * g_scale),
                                };
                                SDL_RenderFillRect(renderer, &sample_rect);
                        }

                        SDL_RenderPresent(renderer);
                }

                SDL_Delay(16);
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
}

void HandleCursorChange() {
        switch (g_usr_selected_mode) {
                case MODE_NONE: case MODE_TYPING: SDL_SetCursor(arrowCursor); break;
                case MODE_PEN: SDL_SetCursor(crosshairCursor); break;
                case MODE_PAN: SDL_SetCursor(panCursor); break;
                case MODE_ERASER: SDL_SetCursor(erasorCursor); break;
        }
}

void DrawGrid(SDL_Renderer* renderer, WindowData win_data) {
        float scaled_grid = win_data.grid_size * g_scale;

        if (scaled_grid >= 1.0f) {
                SDL_SetRenderDrawColor(renderer, unpack_color(g_grid_color));

                int start_x = (int)(g_pan.x / scaled_grid) * win_data.grid_size;
                int end_x = (int)((g_pan.x + win_data.Window_Width) / g_scale);
                for (int x = start_x; x <= end_x; x += win_data.grid_size) {
                        int screen_x = (int)(x * g_scale - g_pan.x);
                        SDL_RenderDrawLine(renderer, screen_x, 0, screen_x, win_data.Window_Height);
                }

                int start_y = (int)(g_pan.y / scaled_grid) * win_data.grid_size;
                int end_y = (int)((g_pan.y + win_data.Window_Height) / g_scale);
                for (int y = start_y; y <= end_y; y += win_data.grid_size) {
                        int screen_y = (int)(y * g_scale - g_pan.y);
                        SDL_RenderDrawLine(renderer, 0, screen_y, win_data.Window_Width, screen_y);
                }
        }
}

SDL_Cursor* createCursorFromPNG(const char* filename, uint8_t width, uint8_t height) {
        SDL_Surface* original = IMG_Load(filename);
        if (!original) {
                SDL_Log("Failed to load PNG: %s", IMG_GetError());
                return NULL;
        }

        // Create a surface with the desired size and same pixel format
        SDL_Surface* resized = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, original->format->format);
        if (!resized) {
                SDL_Log("Failed to create resized surface: %s", SDL_GetError());
                SDL_FreeSurface(original);
                return NULL;
        }

        // Scale original into resized
        if (SDL_BlitScaled(original, NULL, resized, NULL) != 0) {
                SDL_Log("Failed to scale surface: %s", SDL_GetError());
                SDL_FreeSurface(original);
                SDL_FreeSurface(resized);
                return NULL;
        }

        SDL_FreeSurface(original);

        SDL_Cursor* cursor = SDL_CreateColorCursor(resized, width / 2, height / 2);
        if (!cursor) {
                SDL_Log("Failed to create cursor: %s", SDL_GetError());
        }

        SDL_FreeSurface(resized);
        return cursor;
}
