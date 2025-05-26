#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define unpack_color(color) color.r, color.g, color.b, color.a
#define TITLE "Scratch Pad"
#define swap(a, b) \
        do { \
                typeof(*a) temp = *a; \
                *a = *b; \
                *b = temp; \
        } while (0)

const uint16_t CANVAS_WIDTH = 5000;
const uint16_t CANVAS_HEIGHT = 5000;

enum Mode: uint8_t {
        MODE_NONE,
        MODE_PEN,
        MODE_PAN,
        MODE_ERASER,
        MODE_POST_IT,
};

enum Mode user_current_mode = MODE_NONE;

typedef struct {
        int Window_Width, Window_Height;
        int grid_size;
} WindowData;

SDL_Cursor* arrowCursor;
SDL_Cursor* crosshairCursor;
SDL_Cursor* panCursor;
SDL_Cursor* erasorCursor;
void handle_cursor_change(enum Mode current_mode) {
        switch (current_mode) {
                case MODE_NONE: case MODE_POST_IT: SDL_SetCursor(arrowCursor); break;
                case MODE_PEN: SDL_SetCursor(crosshairCursor); break;
                case MODE_PAN: SDL_SetCursor(panCursor); break;
                case MODE_ERASER: SDL_SetCursor(erasorCursor); break;
        }
}

// Button structure
typedef struct {
        SDL_Texture *iconTexture;
        char* iconLocation;
        SDL_Color iconColor;
        char* toolTip;
        SDL_Rect rect;
        SDL_Color bg_color;
        SDL_Color bg_hover_color;
        uint32_t lastTime;
        float SCALE;
        float targetSCALE;
        SDL_bool hovered;
        SDL_bool clicked;
        void* (*onClickFunc)(int btnIndex, char* toolTip);
} Button;

typedef struct {
        Button **btns;
        int toolsCount;
        int toolsCapacity;

        SDL_Rect rect;
        SDL_Color bg_color;

        SDL_Color tool_bg_color;
        SDL_Color tool_hover_color;
        SDL_Color tool_icon_color;

        void* (*onClickFunc)(int btnIndex, char* toolTip);
} ToolBar;

// Global Variable: Buttons
int G_icon_size;
int G_border_radius;
float G_animation_speed;
int G_padding;

Button** btns = NULL;
int btnsCapacity = 0;
int btnsCount = 0;

float SCALE = 1.0f; // TODO: Use in main app
SDL_FPoint pan = {0};
float min_SCALE = 0.1f;

// Function: Button, ToolBar
SDL_Texture* LoadSVGImageAsTexture(const char* path, SDL_Renderer* renderer, int width, int height);
Button* CreateNewButton(SDL_Renderer* renderer, char* iconLocation, char* toolTip, SDL_Rect rect, SDL_Color bg_color, SDL_Color bg_hover_color, SDL_Color iconColor, void* onClickFunc);
void draw_button(SDL_Renderer *renderer, Button *btn);
void draw_all_buttons(SDL_Renderer* renderer);
int mouseInRect(SDL_Rect rect, int x, int y);
void RefactorIconSize(int window_width, int window_height);
void ReloadAllButtons(SDL_Renderer* renderer);
void AllButtonsHoverEventHandler(int mouseX, int mouseY);
void AllButtonsClickEventHandler(void);
void onClick(int btnIndex, char* toolTip);
int AddTool(ToolBar* ToolBar, SDL_Renderer* renderer, char* iconLocation, char* toolTip, const int window_height);
void AddTools(ToolBar* ToolBar, SDL_Renderer* renderer, char* iconLocation[], char* toolTip[], int count, const int window_height);
void reTransformToolBar(ToolBar* toolBar, int window_height);
void drawUI(SDL_Renderer* renderer, ToolBar* toolBar);
void FreeAllButtons(void);
void RepositionAllButtons(SDL_Renderer* renderer, ToolBar* toolBar, Button* menu, Button* undo, Button* redo, Button* zoomIn, Button* zoomOut, int window_width, int window_height);
SDL_Cursor* createCursorFromPNG(const char* filename, uint8_t width, uint8_t height);

int main(void) {
        SDL_Init(SDL_INIT_EVERYTHING);

        WindowData win_data = {
                .Window_Width = 900,
                .Window_Height = 700,
                .grid_size = 50 // Todo: Make it dynamic, MAGIC NUMBER
        };

        setenv("SDL_VIDEODRIVER", "wayland", 1);
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // 2 is for better quality
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, "/dev/input/event*");

        SDL_Color
                bg_button = {.r = 60, .g = 60, .b = 65, .a = 255},
                bg_hover_button = {.r = 80, .g = 80, .b = 90, .a = 255},
                icon_button = {.r = 255, .g = 255, .b = 255, .a = 255},
                grid_color = {.r = 200, .g = 200, .b = 200, .a = 255}; // Grey


        RefactorIconSize(win_data.Window_Width, win_data.Window_Height);

        SDL_Window *window = SDL_CreateWindow(
                TITLE,
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                win_data.Window_Width, win_data.Window_Height,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
        );
        SDL_Renderer *renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC  | SDL_RENDERER_TARGETTEXTURE
        );

        SDL_Delay(1000);
        char cmd[512];
        snprintf(
                cmd,
                sizeof(cmd),
                "hyprctl keyword windowrulev2 'opacity 0.75, title:^(%s)$'",
                TITLE
        );
        system(cmd);

        Button *menu = CreateNewButton(
                renderer,
                "../App_Depencencies/icons/menu.svg",
                "Menu",
                (SDL_Rect) {
                        .x = G_padding * 1.15f,
                        .y = G_padding * 1.15f,
                        .w = G_icon_size,
                        .h = G_icon_size,
                },
                bg_button,
                bg_hover_button,
                icon_button,
                onClick
        );

        ToolBar toolBar = {
                .btns = NULL,
                .toolsCount = 0,
                .toolsCapacity = 0,

                .bg_color = {.r = 215, .g = 213, .b = 201, .a = 255},
                .tool_bg_color = bg_button,
                .tool_hover_color = bg_hover_button,
                .tool_icon_color = icon_button,
                .rect = {.x = -1, .y = -1, .w = 0, .h = 0}, // No tools added yet
        };

        AddTools (
                &toolBar,
                renderer,
                (char*[]) {
                        "../App_Depencencies/icons/tool-pen.svg",
                        "../App_Depencencies/icons/tool-pan.svg",
                        "../App_Depencencies/icons/tool-eraser.svg",
                        "../App_Depencencies/icons/tool-add_note.svg"
                },
                (char*[]) {
                        "Pen Tool",
                        "Pan Tool",
                        "Eraser Tool",
                        "Add Sticky Note"
                },
                4,
                win_data.Window_Height
        );

        Button* undo = CreateNewButton(
                renderer,
                "../App_Depencencies/icons/undo.svg",
                "Undo",
                (SDL_Rect) {
                        .w = G_icon_size,
                        .h = G_icon_size,
                        .x = G_padding,
                        .y = win_data.Window_Height - G_padding - G_icon_size,
                },
                bg_button,
                bg_hover_button,
                icon_button,
                onClick
        );

        Button* redo = CreateNewButton(
                renderer,
                "../App_Depencencies/icons/redo.svg",
                "Redo",
                (SDL_Rect) {
                        .w = G_icon_size,
                        .h = G_icon_size,
                        .x = undo->rect.x + G_icon_size * 1.15f,
                        .y = win_data.Window_Height - G_padding - G_icon_size,
                },
                bg_button,
                bg_hover_button,
                icon_button,
                onClick
        );

        Button* zoomIn = CreateNewButton(
                renderer,
                "../App_Depencencies/icons/zoom-in.svg",
                "Zoom In",
                (SDL_Rect) {
                        .w = G_icon_size,
                        .h = G_icon_size,

                        .x = win_data.Window_Width - G_icon_size - G_padding,
                        .y = win_data.Window_Height - G_padding - G_icon_size,
                },
                bg_button,
                bg_hover_button,
                icon_button,
                onClick
        );

        Button* zoomOut = CreateNewButton(
                renderer,
                "../App_Depencencies/icons/zoom-out.svg",
                "Zoom Out",
                (SDL_Rect) {
                        .w = G_icon_size,
                        .h = G_icon_size,

                        .x = zoomIn->rect.x - G_icon_size * 1.15f,
                        .y = win_data.Window_Height - G_padding - G_icon_size,
                },
                bg_button,
                bg_hover_button,
                icon_button,
                onClick
        );

        SDL_Event event;
        SDL_bool running = SDL_TRUE;
        int mouseX = 0, mouseY = 0;
        SDL_bool redraw = SDL_TRUE;

        pan = (SDL_FPoint) {
                .x = ((float) CANVAS_WIDTH - win_data.Window_Width) / 2.0f,
                .y = ((float) CANVAS_HEIGHT - win_data.Window_Height) / 2.0f
        };

        arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        crosshairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        panCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        erasorCursor = createCursorFromPNG("../App_Depencencies/icons/eraser_cursor.png", 16, 16); // Todo: Fix

        enum Mode current_mode;
        while (running) {
                handle_cursor_change(user_current_mode);

                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT:
                                        running = SDL_FALSE;
                                        break;

                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                        running = SDL_FALSE;
                                                        break;
                                        }
                                        break;

                                case SDL_MOUSEMOTION:
                                        SDL_GetMouseState(&mouseX, &mouseY);
                                        AllButtonsHoverEventHandler(mouseX, mouseY);

                                        if (current_mode == MODE_PAN) {
                                                pan.x -= event.motion.xrel;
                                                pan.y -= event.motion.yrel;

                                                // Clamp pan to never go allow user to go beyond canvas limit
                                                pan.x = SDL_clamp(pan.x, 0, CANVAS_WIDTH * SCALE - win_data.Window_Width);
                                                pan.y = SDL_clamp(pan.y, 0, CANVAS_HEIGHT * SCALE - win_data.Window_Height);
                                        }

                                        redraw = SDL_TRUE;
                                        break;
                                case SDL_MOUSEBUTTONUP:
                                        current_mode = MODE_NONE;
                                        break;

                                case SDL_MOUSEBUTTONDOWN:
                                        current_mode = user_current_mode;
                                        switch(event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        AllButtonsClickEventHandler();
                                                        redraw = SDL_TRUE;
                                                        break;
                                        }
                                        break;
                                case SDL_WINDOWEVENT:
                                        switch (event.window.event) {
                                                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                                                        SDL_GetWindowSize(window, &win_data.Window_Width, &win_data.Window_Height);

                                                        // Basically copy paste function, usefull for cleaning up main() func only
                                                        RepositionAllButtons(renderer, &toolBar, menu, undo, redo, zoomIn, zoomOut, win_data.Window_Width, win_data.Window_Height);


                                                        // Clamp SCALE for new window size
                                                        min_SCALE = fmaxf(
                                                                (float)win_data.Window_Width / CANVAS_WIDTH,
                                                                (float)win_data.Window_Height / CANVAS_HEIGHT
                                                        );
                                                        SCALE = SDL_clamp(SCALE, min_SCALE * 1.01f, 2.0f);

                                                        // Clamp pan to never go allow user to go beyond canvas limit
                                                        pan.x = SDL_clamp(pan.x, 0, CANVAS_WIDTH * SCALE - win_data.Window_Width);
                                                        pan.y = SDL_clamp(pan.y, 0, CANVAS_HEIGHT * SCALE - win_data.Window_Height);

                                                        redraw = SDL_TRUE;
                                                        break;
                                                }
                                        }
                                        break;
                                case SDL_MOUSEWHEEL: {
                                        float oldSCALE = SCALE;

                                        SDL_GetMouseState(&mouseX, &mouseY);
                                        float worldX = (mouseX + pan.x) / SCALE;
                                        float worldY = (mouseY + pan.y) / SCALE;

                                        const float zoomFactor = 1.1f;
                                        if (event.wheel.y != 0) {
                                                SCALE *= powf(zoomFactor, event.wheel.y);
                                        }

                                        // Clamp SCALE
                                        min_SCALE = fmaxf(
                                                (float)win_data.Window_Width / CANVAS_WIDTH,
                                                (float)win_data.Window_Height / CANVAS_HEIGHT
                                        );

                                        SCALE = SDL_clamp(SCALE, min_SCALE * 1.01f, 2.0f); // Max Zoom In: 200%, Min Zoom: dynamic->based on window size

                                        if (oldSCALE != SCALE) {
                                                pan.x = worldX * SCALE - mouseX;
                                                pan.y = worldY * SCALE - mouseY;
                                        }

                                        // Clamp pan to never go allow user to go beyond canvas limit
                                        pan.x = SDL_clamp(pan.x, 0, CANVAS_WIDTH * SCALE - win_data.Window_Width);
                                        pan.y = SDL_clamp(pan.y, 0, CANVAS_HEIGHT * SCALE - win_data.Window_Height);

                                        redraw = SDL_TRUE;
                                        break;
                                }
                        }
                }
                if (redraw) {
                        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
                        SDL_RenderClear(renderer);

                        // Draw Grid
                        float SCALEd_grid = win_data.grid_size * SCALE;
                        if (SCALEd_grid >= 1.0f) {
                                SDL_SetRenderDrawColor(renderer, unpack_color(grid_color));

                                int start_x = (int)(pan.x /
                                        SCALEd_grid) * win_data.grid_size;
                                int end_x = (int)((pan.x + win_data.Window_Width) / SCALE);
                                for (int x = start_x; x <= end_x; x += win_data.grid_size) {
                                        int screen_x = (int)(x * SCALE - pan.x);
                                        SDL_RenderDrawLine(renderer, screen_x, 0, screen_x, win_data.Window_Height);
                                }

                                int start_y = (int)(pan.y / SCALEd_grid) * win_data.grid_size;
                                int end_y = (int)((pan.y + win_data.Window_Height) / SCALE);
                                for (int y = start_y; y <= end_y; y += win_data.grid_size) {
                                        int screen_y = (int)(y * SCALE - pan.y);
                                        SDL_RenderDrawLine(renderer, 0, screen_y, win_data.Window_Width, screen_y);
                                }
                        }

                        drawUI(renderer, &toolBar);
                        SDL_RenderPresent(renderer);
                        redraw = SDL_FALSE;
                }

        }

        // Cleanup
        FreeAllButtons();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
}

SDL_Texture* LoadSVGImageAsTexture(const char* path, SDL_Renderer* renderer, int width, int height) {
        SDL_RWops* rw = SDL_RWFromFile(path, "rb");
        if (!rw) {
                printf("RWFromFile Error: %s\n", SDL_GetError());
                return NULL;
        }

        SDL_Surface* surface = IMG_LoadSizedSVG_RW(rw, width, height);
        SDL_RWclose(rw);

        if (!surface) {
                printf("Image Load Error: %s\n", IMG_GetError());
                return NULL;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        return texture;
}

void draw_button(SDL_Renderer *renderer, Button *btn) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - btn->lastTime) / 1000.0f;
        btn->lastTime = now;

        // Smooth scaling
        btn->SCALE += (btn->targetSCALE - btn->SCALE) * fminf(G_animation_speed * dt, 1.0f);

        SDL_Rect r = btn->rect;
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        r.w *= btn->SCALE;
        r.h *= btn->SCALE;
        r.x = cx - r.w / 2;
        r.y = cy - r.h / 2;

        SDL_Color fill = btn->clicked ? btn->bg_hover_color : (btn->hovered ? btn->bg_hover_color : btn->bg_color);

        roundedBoxRGBA(renderer, r.x, r.y, r.x + r.w, r.y + r.h, G_border_radius, fill.r, fill.g, fill.b, fill.a);
        roundedRectangleRGBA(renderer, r.x, r.y, r.x + r.w, r.y + r.h, G_border_radius, 0, 0, 0, 255);

        if (btn->clicked) {
                roundedRectangleRGBA(renderer, r.x + 1, r.y + 1, r.x + r.w - 1, r.y + r.h - 1, G_border_radius, 0, 255, 255, 255);
        } else {
                roundedRectangleRGBA(renderer, r.x + 1, r.y + 1, r.x + r.w - 1, r.y + r.h - 1, G_border_radius, 255, 255, 255, 255);
        }

        if (btn->iconTexture) {
                SDL_SetTextureColorMod(btn->iconTexture, btn->iconColor.r, btn->iconColor.g, btn->iconColor.b);
                int icon_size = (r.w < r.h ? r.w : r.h) * 2 / 3;
                SDL_Rect iconRect = {
                        .x = r.x + r.w / 2 - icon_size / 2,
                        .y = r.y + r.h / 2 - icon_size / 2,
                        .w = icon_size,
                        .h = icon_size
                };
                SDL_RenderCopy(renderer, btn->iconTexture, NULL, &iconRect);
        }
}

void RefactorIconSize(int window_width, int window_height) {
        int min_dim = window_width < window_height ? window_width : window_height;

        // Map [400, 1800] to [42, 64]
        float t = (min_dim - 400) / (1800.0f - 400.0f);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        G_icon_size = (int)(42 + t * (64 - 42));

        G_border_radius = G_icon_size / 5;
        G_animation_speed = (float)G_icon_size / 12.0f;
        G_padding = G_icon_size / 2;
}

Button* CreateNewButton(SDL_Renderer* renderer, char* iconLocation, char* toolTip, SDL_Rect rect, SDL_Color bg_color, SDL_Color bg_hover_color, SDL_Color iconColor, void* onClickFunc) {
        // TODO: Make sure toolTip is unique
        if (btnsCount >= btnsCapacity) {
                btnsCapacity = (btnsCapacity == 0) ? 1 : btnsCapacity * 2;
                btns = realloc(btns, btnsCapacity * sizeof(Button*));
                if (!btns) {
                        // Failed to realloc
                        return NULL;
                }
        }

        Button *new_button = malloc(sizeof(Button));
        if (!new_button) return NULL;

        new_button->iconTexture = LoadSVGImageAsTexture(iconLocation, renderer, G_icon_size, G_icon_size);
        new_button->iconLocation = strdup(iconLocation); // Todo: free all strdup
        new_button->iconColor = iconColor;

        new_button->toolTip = strdup(toolTip);

        new_button->rect = rect;

        new_button->bg_color = bg_color;
        new_button->bg_hover_color = bg_hover_color;

        new_button->lastTime = SDL_GetTicks();
        new_button->SCALE = 1.0f;
        new_button->targetSCALE = 1.0f;
        new_button->hovered = SDL_FALSE;
        new_button->clicked = SDL_FALSE;
        new_button->onClickFunc = onClickFunc;

        // Store address of newButton
        btns[btnsCount] = new_button;
        btnsCount++;

        // return new_button;
        return new_button;
}

void ReloadAllButtons(SDL_Renderer* renderer) {
        for (int i = 0; i < btnsCount; i++) {
                SDL_DestroyTexture(btns[i]->iconTexture);

                btns[i]->iconTexture = LoadSVGImageAsTexture(btns[i]->iconLocation, renderer, G_icon_size, G_icon_size);

                btns[i]->rect.w = G_icon_size;
                btns[i]->rect.h = G_icon_size;
        }
}

void FreeAllButtons(void) {
        for (int i = 0; i < btnsCount; i++) {
                if (btns[i] != NULL) {
                        free(btns[i]);
                }
        }
        free(btns);
}

int mouseInRect(SDL_Rect rect, int x, int y) {
        return x >= rect.x && x <= (rect.x + rect.w) && y >= rect.y && y <= (rect.y + rect.h);
}

void AllButtonsHoverEventHandler(int mouseX, int mouseY) {
        for (int i = btnsCount - 1; i >= 0; i--) {
                btns[i]->hovered = mouseInRect(btns[i]->rect, mouseX, mouseY);
                btns[i]->targetSCALE = btns[i]->hovered? 1.1f: 1.0f;
        }
}

void AllButtonsClickEventHandler(void) {
        int i = -1;
        for (i = btnsCount - 1; i >= 0; i--) {
                if (btns[i]->hovered) {
                        btns[i]->clicked = !btns[i]->clicked;
                        btns[i]->onClickFunc(i, btns[i]->toolTip);
                        break;
                }
        }

        // Unclick all other button
        for (int j = 0; j < btnsCount; j++) {
                if (j != i && i >= 0) {
                        btns[j]->clicked = SDL_FALSE;
                }
        }
}

void draw_all_buttons(SDL_Renderer* renderer) {
        for (int i = 0; i < btnsCount; i++) {
                draw_button(renderer, btns[i]);
        }
}

int AddTool(ToolBar* toolBar, SDL_Renderer* renderer, char* iconLocation, char* toolTip, const int window_height) {
        // Auto Rect
        if (toolBar->toolsCount >= toolBar->toolsCapacity) {
                toolBar->toolsCapacity = (toolBar->toolsCapacity == 0) ? 1 : toolBar->toolsCapacity * 2;
                toolBar->btns = realloc(toolBar->btns, toolBar->toolsCapacity * sizeof(Button*));
                if (!btns) {
                        // Failed to realloc
                        return -1;
                }
        }

        // Calculate layout
        float toolPadding = 0.15f * G_icon_size;
        int btnCount = toolBar->toolsCount + 1;
        float totalHeight = btnCount * G_icon_size + toolPadding * (btnCount + 1);

        // Center toolbar vertically in window
        toolBar->rect.x = G_padding;
        toolBar->rect.y = (window_height - totalHeight) / 2;
        toolBar->rect.w = G_icon_size + 2 * toolPadding;
        toolBar->rect.h = totalHeight;

        // Refactor previous buttons of the toolbar
        for (int i = 0; i < toolBar->toolsCount; i++) {
                toolBar->btns[i]->rect.y = toolBar->rect.y + toolPadding + i * (G_icon_size + toolPadding);
        }

        // Center new button inside the toolbar
        SDL_Rect new_btn_rect = {
                .w = G_icon_size,
                .h = G_icon_size,
                .x = toolBar->rect.x + (toolBar->rect.w - G_icon_size) / 2,
                .y = toolBar->rect.y + toolPadding + toolBar->toolsCount * (G_icon_size + toolPadding),
        };

        Button* new_btn = CreateNewButton(
                renderer,
                iconLocation,
                toolTip,
                new_btn_rect,
                toolBar->tool_bg_color,
                toolBar->tool_hover_color,
                toolBar->tool_icon_color,
                onClick
        );

        if (!new_btn) {
                return -1; // Failure
        }

        toolBar->btns[toolBar->toolsCount] = new_btn;
        toolBar->toolsCount++;

        return 0; // Successfull
}

void AddTools(ToolBar* toolBar, SDL_Renderer* renderer, char* iconLocation[], char* toolTip[], int count, const int window_height) {
        for (int i = 0; i < count; i++) {
                AddTool(toolBar, renderer, iconLocation[i], toolTip[i], window_height);
        }
}

void reTransformToolBar(ToolBar* toolBar, int window_height) {
        float toolPadding = 0.15f * G_icon_size;

        // Refactor ToolBar (w, h, x, y)
        toolBar->rect.x = G_padding;
        toolBar->rect.w = G_icon_size + 2 * toolPadding;
        toolBar->rect.h = (toolBar->toolsCount) * G_icon_size + toolPadding * (toolBar->toolsCount + 1);
        toolBar->rect.y = (window_height - toolBar->rect.h) / 2;

        // Refactor all buttons within:
        for (int i = 0; i < toolBar->toolsCount; i++) {
                toolBar->btns[i]->rect.x = G_padding + toolPadding;
                toolBar->btns[i]->rect.y = toolBar->rect.y + toolPadding + i * (G_icon_size + toolPadding);
        }
}

void onClick(int btnIndex, char* toolTip) {
        if (btns[btnIndex]->clicked) {
                printf("Clicked %s\n", toolTip);
        } else {
                printf("Unclicked %s\n", toolTip);
        }

        if (strcmp(toolTip, "Eraser Tool") == 0) {
                if (btns[btnIndex]->clicked) {
                        user_current_mode = MODE_ERASER;
                } else {
                        user_current_mode = MODE_NONE;
                }
        } else if (strcmp(toolTip, "Pen Tool") == 0) {
                if (btns[btnIndex]->clicked) {
                        user_current_mode = MODE_PEN;
                } else {
                        user_current_mode = MODE_NONE;
                }
        } else if (strcmp(toolTip, "Pan Tool") == 0) {
                if (btns[btnIndex]->clicked) {
                        user_current_mode = MODE_PAN;
                } else {
                        user_current_mode = MODE_NONE;
                }
        } else if (strcmp(toolTip, "Reset Zoom") == 0) {
                btns[btnIndex]->clicked = SDL_FALSE;
                SCALE = 1.0f;
        } else if (strcmp(toolTip, "Zoom Out") == 0) {
                btns[btnIndex]->clicked = SDL_FALSE;
                SCALE -= 0.1f;
                SCALE = SDL_clamp(SCALE, min_SCALE * 1.01f, 2.0f);
        } else if (strcmp(toolTip, "Zoom In") == 0) {
                btns[btnIndex]->clicked = SDL_FALSE;
                SCALE += 0.1f;
                SCALE = SDL_clamp(SCALE, min_SCALE * 1.01f, 2.0f);
        } else if (strcmp(toolTip, "Undo") == 0) {
                // TODO:
                btns[btnIndex]->clicked = SDL_FALSE;
        } else if (strcmp(toolTip, "Redo") == 0) {
                // TODO:
                btns[btnIndex]->clicked = SDL_FALSE;
        }
}

void RepositionAllButtons(SDL_Renderer* renderer, ToolBar* toolBar, Button* menu, Button* undo, Button* redo, Button* zoomIn, Button* zoomOut, int window_width, int window_height) {
        RefactorIconSize(window_width, window_height); // G-icon-size modifies according to current window dimension
        ReloadAllButtons(renderer);

        // Repostion Menu
        menu->rect.x = G_padding * 1.15f;
        menu->rect.y = G_padding * 1.15f;

        // Reposition ToolBar
        reTransformToolBar(toolBar, window_height);

        // Redo/Undo and SCALE up/down Reposition:
        undo->rect.x = G_padding;
        redo->rect.x = undo->rect.x + G_icon_size * 1.15f;
        zoomIn->rect.y =
                zoomOut->rect.y =
                        redo->rect.y =
                                undo->rect.y = window_height - G_padding - G_icon_size;
        zoomIn->rect.x = window_width - G_icon_size - G_padding;
        zoomOut->rect.x = zoomIn->rect.x - G_icon_size * 1.15f;
}

void drawUI(SDL_Renderer* renderer, ToolBar* toolBar) {
        roundedBoxRGBA(renderer, toolBar->rect.x, toolBar->rect.y, toolBar->rect.x + toolBar->rect.w, toolBar->rect.y + toolBar->rect.h, G_border_radius, unpack_color(toolBar->bg_color));
        draw_all_buttons(renderer);
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
