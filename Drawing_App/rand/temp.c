
#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600
#define STEPS 100

typedef struct {
    double x, y;
    int thickness;
} Point;

// Bézier & derivative
Point bezier(double t, Point p0, Point p1, Point p2) {
    double u = 1 - t;
    Point p;
    p.x = u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x;
    p.y = u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y;
    return p;
}

Point bezier_derivative(double t, Point p0, Point p1, Point p2) {
    Point d;
    d.x = 2 * (1 - t) * (p1.x - p0.x) + 2 * t * (p2.x - p1.x);
    d.y = 2 * (1 - t) * (p1.y - p0.y) + 2 * t * (p2.y - p1.y);
    return d;
}

// Normalize vector
void normalize(Point *p) {
    double len = sqrt(p->x * p->x + p->y * p->y);
    if (len != 0) {
        p->x /= len;
        p->y /= len;
    }
}

// Rotate vector 90 degrees CCW
Point normal(Point tangent) {
    Point n = {-tangent.y, tangent.x};
    normalize(&n);
    return n;
}

// Thickness function
double thickness(double t) {
    return 2.0 + 18.0 * t; // Vary 2 → 20
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Manual Variable Thickness Stroke",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Surface *screen_surface = SDL_GetWindowSurface(window);
    SDL_Surface *cairo_surface_sdl = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char *)cairo_surface_sdl->pixels,
        CAIRO_FORMAT_ARGB32,
        cairo_surface_sdl->w, cairo_surface_sdl->h,
        cairo_surface_sdl->pitch
    );
    cairo_t *cr = cairo_create(cairo_surface);

    // White background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Control points
    Point p0 = {100, 500};
    Point p1 = {400, 100};
    Point p2 = {700, 500};

    // Stroke edge points
    Point left[STEPS + 1];
    Point right[STEPS + 1];

    for (int i = 0; i <= STEPS; ++i) {
        double t = (double)i / STEPS;
        Point pt = bezier(t, p0, p1, p2);
        Point tangent = bezier_derivative(t, p0, p1, p2);
        Point n = normal(tangent);
        double w = thickness(t) / 2.0;

        left[i].x = pt.x + n.x * w;
        left[i].y = pt.y + n.y * w;
        right[i].x = pt.x - n.x * w;
        right[i].y = pt.y - n.y * w;
    }

    // Draw filled polygon
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, left[0].x, left[0].y);
    for (int i = 1; i <= STEPS; ++i)
        cairo_line_to(cr, left[i].x, left[i].y);
    for (int i = STEPS; i >= 0; --i)
        cairo_line_to(cr, right[i].x, right[i].y);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_destroy(cr);
    cairo_surface_flush(cairo_surface);
    cairo_surface_destroy(cairo_surface);

    SDL_BlitSurface(cairo_surface_sdl, NULL, screen_surface, NULL);
    SDL_UpdateWindowSurface(window);

    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = 1;
        }
        SDL_Delay(10);
    }

    SDL_FreeSurface(cairo_surface_sdl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
