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

int addPoint(LinesArray* PA, int32_t x, int32_t y, uint8_t line_thickness, bool connected_to_prev_line) {
        if (PA->pointCapacity >= UINT16_MAX) {
                return 1;
        }

        if (PA->pointCount >= PA->pointCapacity) {
                uint16_t new_capacity = (PA->pointCapacity == 0) ? 1 : PA->pointCapacity * 2;

                if (new_capacity >= UINT16_MAX) {
                        new_capacity = UINT16_MAX;
                }

                Point* temp = realloc(PA->points, new_capacity * sizeof(Point));
                if (!temp) {
                        fprintf(stderr, "Memory allocation failed!\n");
                        return 1;
                }
                PA->points = temp;
                PA->pointCapacity = new_capacity;
        }

        bool add_point = true;

        if (connected_to_prev_line && PA->pointCount > 0) {
                int32_t dx = PA->points[PA->pointCount - 1].x - x;
                int32_t dy = PA->points[PA->pointCount - 1].y - y;
                add_point = (dx * dx + dy * dy) > (POINTS_THRESHOLD * POINTS_THRESHOLD);
        }

        if (add_point) {
                PA->points[PA->pointCount].x = x;
                PA->points[PA->pointCount].y = y;
                PA->points[PA->pointCount].connected_to_next_point = connected_to_prev_line;
                PA->points[PA->pointCount].line_thickness = line_thickness;
                PA->pointCount++;
        }

        return 0;
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
        if (p1.connected_to_next_point && p2.connected_to_next_point) {
                BetterLine(renderer, p1.x + pan.x, p1.y + pan.y, p2.x + pan.x, p2.y + pan.y, color);
        }
}

uint16_t rendered_till = 0;
void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA, Pan pan, SDL_Color color) {
        if (PA->pointCount != 0) {
                for (uint16_t i = 0; i < (PA -> pointCount - 1); i++) {
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
                rendered_till += 1;
        }
}

void PanPoints(Pan* pan, float xrel, float yrel) {
        pan->x += xrel;
        pan->y += yrel;
}

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

void douglasPeucker(Point* points, int start, int end, double epsilon, int* keep) {
        if (end <= start + 1) {
                return;
        }

        double maxDist = 0.0;
        int index = start;

        for (int i = start + 1; i < end; ++i) {
                double dist = perpendicularDistance(points[i], points[start], points[end]);
                if (dist > maxDist) {
                        maxDist = dist;
                        index = i;
                }
        }

        if (maxDist > epsilon) {
                keep[index] = 1;
                douglasPeucker(points, start, index, epsilon, keep);
                douglasPeucker(points, index, end, epsilon, keep);
        }
}

void OptimizeLine(LinesArray* PA, uint16_t line_start_index, uint16_t line_end_index, double epsilon) {
        int* keep = calloc(PA->pointCount, sizeof(int));
        keep[line_start_index] = 1;
        keep[line_end_index] = 1;

        douglasPeucker(PA->points, line_start_index, line_end_index, epsilon, keep);

        uint16_t temp = 0;
        for (int i = line_start_index; i <= line_end_index; ++i) {
                if (keep[i]) {
                        PA->points[line_start_index + temp] = PA->points[i];
                        temp += 1;
                }
        }

        PA->pointCount = line_start_index + temp;
        rendered_till = PA->pointCount;
        free(keep);
}
