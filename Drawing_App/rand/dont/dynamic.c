#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

// Custom context data for the button
typedef struct {
        const char* label;
        int id;
} ButtonContext;

// Button struct with callbacks and context
typedef struct {
        SDL_Rect rect;
        void (*onhover)(void*);
        void (*onclick)(void*);
        void* context;
        bool hovering;
} Button;

// Check if mouse is inside the button
bool point_in_rect(int x, int y, SDL_Rect* r) {
        return x >= r->x && x <= (r->x + r->w) && y >= r->y && y <= (r->y + r->h);
}

// Callback functions
void say_hello(void* ctx) {
        ButtonContext* data = (ButtonContext*)ctx;
        printf("Clicked: Hello from Button '%s' with id %d\n", data->label, data->id);
}

void hover_msg(void* ctx) {
        ButtonContext* data = (ButtonContext*)ctx;
        printf("Hovering over Button '%s'\n", data->label);
}

int main() {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Window* window = SDL_CreateWindow("OOP SDL2 Button",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                640, 480, SDL_WINDOW_SHOWN);
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

        // Button context
        ButtonContext btn_ctx = { .label = "Submit", .id = 101 };

        // Setup button
        Button btn = {
                .rect = {200, 200, 200, 60},
                .onhover = hover_msg,
                .onclick = say_hello,
                .context = &btn_ctx,
                .hovering = false
        };

        SDL_Event e;
        bool running = true;

        while (running) {
                while (SDL_PollEvent(&e)) {
                        switch (e.type) {
                        case SDL_QUIT:
                                running = false;
                                break;

                        case SDL_MOUSEMOTION: {
                                int x = e.motion.x;
                                int y = e.motion.y;
                                bool is_hovering = point_in_rect(x, y, &btn.rect);

                                if (is_hovering && !btn.hovering) {
                                        btn.hovering = true;
                                        if (btn.onhover) btn.onhover(btn.context);
                                } else if (!is_hovering) {
                                        btn.hovering = false;
                                }
                                break;
                        }

                        case SDL_MOUSEBUTTONDOWN:
                                if (e.button.button == SDL_BUTTON_LEFT &&
                                    point_in_rect(e.button.x, e.button.y, &btn.rect)) {
                                        if (btn.onclick) btn.onclick(btn.context);
                                }
                                break;
                        }
                }

                // Render background
                SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
                SDL_RenderClear(renderer);

                // Render button
                SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
                SDL_RenderFillRect(renderer, &btn.rect);

                SDL_RenderPresent(renderer);
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
}
