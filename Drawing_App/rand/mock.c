#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#define WIDTH 800
#define HEIGHT 600

// Function to draw a thick line using Cairo
void draw_thick_line(cairo_t *cr, double x1, double y1, double x2, double y2, double thickness) {
    cairo_set_line_width(cr, thickness);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

// Function to draw a Bézier curve using Cairo
void draw_bezier_curve(cairo_t *cr, double x1, double y1, double x2, double y2,
                       double x3, double y3, double x4, double y4) {
    cairo_move_to(cr, x1, y1);
    cairo_curve_to(cr, x2, y2, x3, y3, x4, y4);
    cairo_stroke(cr);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    // Create SDL window
    SDL_Window *window = SDL_CreateWindow("Cairo with SDL2",
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         WIDTH, HEIGHT,
                                         SDL_WINDOW_SHOWN);

    // Create SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                               SDL_RENDERER_ACCELERATED |
                                               SDL_RENDERER_PRESENTVSYNC);

    // Create SDL texture that will be used as Cairo surface
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                           SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           WIDTH, HEIGHT);

    // Create Cairo surface
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t *cr = cairo_create(surface);

    // Main loop flag
    int running = 1;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Clear Cairo surface
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_paint(cr);

        // Set drawing color to black
        cairo_set_source_rgba(cr, 0, 0, 0, 1);

        // 1. Draw a simple line
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, 100, 100);
        cairo_line_to(cr, 200, 200);
        cairo_stroke(cr);

        // 2. Draw a thick line
        draw_thick_line(cr, 300, 100, 400, 200, 5.0);

        // 3. Draw a Bézier curve
        draw_bezier_curve(cr, 100, 300, 150, 250, 250, 350, 300, 300);

        // Update SDL texture with Cairo surface data
        void *pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, cairo_image_surface_get_data(surface), WIDTH * HEIGHT * 4);
        SDL_UnlockTexture(texture);

        // Render the texture
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
