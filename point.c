#include "point.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
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


// Set pixel with intensity blending
void setPixel(SDL_Renderer* renderer, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a, float intensity) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(a * intensity));
        SDL_RenderDrawPoint(renderer, x, y);
}

// Improved Wu's Anti-Aliased Line Algorithm with Thickness
void better_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, uint8_t thickness, SDL_Color color) {
        int steep = abs(y2 - y1) > abs(x2 - x1);

        if (steep) {
                swap(&x1, &y1);
                swap(&x2, &y2);
        }

        if (x1 > x2) {
                swap(&x1, &x2);
                swap(&y1, &y2);
        }

        float dx = (float)(x2 - x1);
        float dy = (float)(y2 - y1);
        float gradient = (dx == 0.0) ? 1.0 : dy / dx;

        // Calculate perpendicular gradient for thickness
        float perpendicular_gradient = (gradient == 0.0) ? 1.0 : -1.0 / gradient;

        // Loop through a range of thickness levels to draw a thick line
        for (int t = -(thickness / 2); t <= (thickness / 2); t++) {
                // Calculate offsets for the current thickness level
                float offset_x = t * cos(atan(perpendicular_gradient));
                float offset_y = t * sin(atan(perpendicular_gradient));

                // Adjust the starting and ending points based on the thickness offset
                int adjusted_x1 = x1 + offset_x;
                int adjusted_y1 = y1 + offset_y;
                int adjusted_x2 = x2 + offset_x;
                int adjusted_y2 = y2 + offset_y;

                // Recalculate gradient for the adjusted line
                float adjusted_dx = adjusted_x2 - adjusted_x1;
                float adjusted_dy = adjusted_y2 - adjusted_y1;
                float adjusted_gradient = (adjusted_dx == 0.0) ? 1.0 : adjusted_dy / adjusted_dx;
                float intery = adjusted_y1 + adjusted_gradient * (adjusted_x1 - x1);

                // Draw the first pixel
                int xend = adjusted_x1;
                int yend = round(adjusted_y1);
                float xgap = 1 - (adjusted_x1 + 0.5 - floor(adjusted_x1 + 0.5));
                int xpxl1 = xend;
                int ypxl1 = yend;

                if (steep) {
                        setPixel(renderer, ypxl1, xpxl1, unpack_color(color), (1 - (intery - floor(intery))) * xgap);
                        setPixel(renderer, ypxl1 + 1, xpxl1, unpack_color(color), (intery - floor(intery)) * xgap);
                } else {
                        setPixel(renderer, xpxl1, ypxl1, unpack_color(color), (1 - (intery - floor(intery))) * xgap);
                        setPixel(renderer, xpxl1, ypxl1 + 1, unpack_color(color), (intery - floor(intery)) * xgap);
                }

                intery += adjusted_gradient;

                // Draw the middle pixels
                for (int x = xpxl1 + 1; x < adjusted_x2; x++) {
                        int y = floor(intery);
                        float f = intery - y;

                        if (steep) {
                                setPixel(renderer, y, x, unpack_color(color), 1 - f);
                                setPixel(renderer, y + 1, x, unpack_color(color), f);
                        } else {
                                setPixel(renderer, x, y, unpack_color(color), 1 - f);
                                setPixel(renderer, x, y + 1, unpack_color(color), f);
                        }
                        intery += adjusted_gradient;
                }
        }
}

Point Lerp(const Point a, const Point b, const float t) {
        Point point = {
                .connected_to_previous_point = true,
                .line_thickness = (int) ((a.line_thickness + b.line_thickness) / 2),
        };


        point.x = a.x + (b.x - a.x) * t;
        point.y = a.y + (b.y - a.y) * t;

        return point;
}

void RenderLine(SDL_Renderer* renderer, Point p1, Point p2, Pan pan, SDL_Color color) {
        if (p1.connected_to_previous_point && p2.connected_to_previous_point) {
                SDL_SetRenderDrawColor(renderer, unpack_color(color));
                better_line(renderer, p1.x + pan.x, p1.y + pan.y, p2.x + pan.x, p2.y + pan.y, (p1.line_thickness + p1.line_thickness) / 2, color);
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
