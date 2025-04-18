#include "point.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)
#define swap(a, b) \
        do { \
                typeof(*a) temp = *a; \
                *a = *b; \
                *b = temp; \
        } while (0)
#define POINTS_THRESHOLD 1

void addPoint(LinesArray* PA, int32_t x, int32_t y, uint8_t line_thickness, bool connected_to_prev_line) {
        if (PA->pointCapacity >= UINT16_MAX) {
                return;
        }

        if (PA->pointCount >= PA->pointCapacity) {
                PA->pointCapacity = (PA->pointCapacity == 0) ? 1 : PA->pointCapacity * 2;

                if (PA->pointCapacity >= UINT16_MAX) {
                        PA->pointCapacity = UINT16_MAX;
                }

                Point* temp = realloc(PA->points, PA->pointCapacity * sizeof(Point));
                if (!temp) {
                        fprintf(stderr, "Memory allocation failed!\n");
                        free(PA->points); // Free existing memory
                        exit(1);
                }
                PA->points = temp;
        }

        bool add_point = true;

        add_point = !connected_to_prev_line ? true: sqrt((PA->points[PA->pointCount - 1].x - x) * (PA->points[PA->pointCount - 1].x - x) + (PA->points[PA->pointCount - 1].y - y) * (PA->points[PA->pointCount - 1].y - y)) > POINTS_THRESHOLD ? true: false;

        if (add_point){
                PA->points[PA->pointCount].x = x;
                PA->points[PA->pointCount].y = y;
                PA->points[PA->pointCount].connected_to_previous_point = connected_to_prev_line;
                PA->points[PA->pointCount].line_thickness = line_thickness;
                PA->pointCount++;
        }
}

void setPixel(SDL_Renderer* renderer, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a, float intensity) {
        SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * intensity));
        SDL_RenderDrawPoint(renderer, x, y);
}

float fractionalPart(float x) {
        return x - floorf(x);
}

float rfpart(float x) {
        return 1.0f - fractionalPart(x);
}

// Wu's Algorithm: Wikipedia
void BetterLine(SDL_Renderer* renderer, int x0, int y0, int x1, int y1, SDL_Color color) {
        bool steep = abs(y1 - y0) > abs(x1 - x0);

        // Swap if the line is steep
        if (steep) {
                swap(&x0, &y0);
                swap(&x1, &y1);
        }

        // Ensure x0 < x1
        if (x0 > x1) {
                swap(&x0, &x1);
                swap(&y0, &y1);
        }

        float dx = x1 - x0;
        float dy = y1 - y0;
        float gradient = (dx == 0.0f) ? 1.0f : dy / dx;

        // First endpoint calculation
        float xend = x0;
        float yend = y0 + gradient * (xend - x0);
        float xgap = rfpart(x0 + 0.5f);
        int xpxl1 = (int)xend;
        int ypxl1 = (int)floorf(yend);

        if (steep) {
                setPixel(renderer, ypxl1, xpxl1, unpack_color(color), rfpart(yend) * xgap);
                setPixel(renderer, ypxl1 + 1, xpxl1, unpack_color(color), fractionalPart(yend) * xgap);
        } else {
                setPixel(renderer, xpxl1, ypxl1, unpack_color(color), rfpart(yend) * xgap);
                setPixel(renderer, xpxl1, ypxl1 + 1, unpack_color(color), fractionalPart(yend) * xgap);
        }

        float intery = yend + gradient;

        // Second endpoint calculation
        xend = x1;
        yend = y1 + gradient * (xend - x1);
        xgap = fractionalPart(x1 + 0.5f);
        int xpxl2 = (int)xend;
        int ypxl2 = (int)floorf(yend);

        if (steep) {
                setPixel(renderer, ypxl2, xpxl2, unpack_color(color), rfpart(yend) * xgap);
                setPixel(renderer, ypxl2 + 1, xpxl2, unpack_color(color), fractionalPart(yend) * xgap);
        } else {
                setPixel(renderer, xpxl2, ypxl2, unpack_color(color), rfpart(yend) * xgap);
                setPixel(renderer, xpxl2, ypxl2 + 1, unpack_color(color), fractionalPart(yend) * xgap);
        }

        // Main loop
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
                int y = (int)floorf(intery);
                float fpart = fractionalPart(intery);

                if (steep) {
                        setPixel(renderer, y, x, unpack_color(color), 1.0f - fpart);
                        setPixel(renderer, y + 1, x, unpack_color(color), fpart);
                } else {
                        setPixel(renderer, x, y, unpack_color(color), 1.0f - fpart);
                        setPixel(renderer, x, y + 1, unpack_color(color), fpart);
                }

                intery += gradient;
        }
}


void RenderLine(SDL_Renderer* renderer, Point p1, Point p2, Pan pan, SDL_Color color) {
        if (p1.connected_to_previous_point && p2.connected_to_previous_point) {
                BetterLine(renderer, p1.x + pan.x, p1.y + pan.y, p2.x + pan.x, p2.y + pan.y, color);
        }
}

uint16_t rendered_till = 0;
void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA, Pan pan, SDL_Color color) {
        if (PA->pointCount != 0) {
                for (uint16_t i = 0; i < PA -> pointCount - 1; i++) {
                        RenderLine(renderer, PA->points[i], PA->points[i + 1], pan, color);
                }
        }
        rendered_till = PA->pointCount;
}

void RenderLines(SDL_Renderer* renderer, LinesArray* PA, Pan pan, SDL_Color color) {
        if (PA->pointCount == 0 || PA == NULL) return;

        if (rendered_till > PA->pointCount) {
            rendered_till = PA->pointCount;
        }

        while (rendered_till < PA->pointCount - 1) {
                RenderLine(renderer, PA->points[rendered_till], PA->points[rendered_till + 1], pan, color);
                rendered_till ++;
        }
}

void PanPoints(Pan* pan, float xrel, float yrel) {
        pan->x += xrel;
        pan->y += yrel;
}
