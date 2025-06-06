#include "point.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL2_gfxPrimitives.h>
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
#define fpart(x) ((x) - (int)(x))
#define rfpart(x) (1.0f - fpart(x))

static int SCREEN_WIDTH, SCREEN_HEIGHT;
void set_window_dimensions(int win_width, int win_height) {
        SCREEN_WIDTH = win_width;
        SCREEN_HEIGHT = win_height;
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

int addPoint(LinesArray* PA, float x, float y, uint8_t line_thickness, bool connected_to_prev_line) {
        if (PA->pointCapacity >= UINT16_MAX) {
                return 1;
        }

        if (PA->pointCount >= PA->pointCapacity) {
                uint16_t new_capacity = (PA->pointCapacity == 0) ? 1 : PA->pointCapacity << 1;

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

        PA->points[PA->pointCount].x = x;
        PA->points[PA->pointCount].y = y;
        PA->points[PA->pointCount].connected_to_next_point = connected_to_prev_line;
        PA->points[PA->pointCount].line_thickness = line_thickness;
        PA->pointCount++;

        return 0;
}

void setPixel(SDL_Renderer* renderer, float x, float y, SDL_Color color, float intensity) {
        color.a *= intensity;

        SDL_SetRenderDrawBlendMode(renderer, (color.a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, unpack_color(color));
        SDL_RenderDrawPointF(renderer, x, y);
}

// Wu's Algorithm: Wikipedia
void BetterLine(SDL_Renderer* renderer, float x0, float y0, float x1, float y1, SDL_Color color) {
        if ((x0 < 0 && x1 < 0) || (x0 > SCREEN_WIDTH && x1 > SCREEN_WIDTH) ||
            (y0 < 0 && y1 < 0) || (y0 > SCREEN_HEIGHT && y1 > SCREEN_HEIGHT)) {
                return;  // Line is outside the screen
        }

        bool steep = fabs(y1 - y0) > fabs(x1 - x0);

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
        float gradient = (fabs(dx) < 1e-6) ? 1.0f : dy / dx;

        // First endpoint calculation
        float xend = x0;
        float yend = y0 + gradient * (xend - x0);
        float xgap = rfpart(x0 + 0.5f);
        int xpxl1 = (int)xend;
        int ypxl1 = (int)floorf(yend);

        if (steep) {
                setPixel(renderer, ypxl1, xpxl1, color, rfpart(yend) * xgap);
                setPixel(renderer, ypxl1 + 1, xpxl1, color, fpart(yend) * xgap);
        } else {
                setPixel(renderer, xpxl1, ypxl1, color, rfpart(yend) * xgap);
                setPixel(renderer, xpxl1, ypxl1 + 1, color, fpart(yend) * xgap);
        }

        float intery = yend + gradient;

        // Second endpoint calculation
        xend = x1;
        yend = y1 + gradient * (xend - x1);
        xgap = fpart(x1 + 0.5f);
        int xpxl2 = (int)xend;
        int ypxl2 = (int)floorf(yend);

        if (steep) {
                setPixel(renderer, ypxl2, xpxl2, color, rfpart(yend) * xgap);
                setPixel(renderer, ypxl2 + 1, xpxl2, color, fpart(yend) * xgap);
        } else {
                setPixel(renderer, xpxl2, ypxl2, color, rfpart(yend) * xgap);
                setPixel(renderer, xpxl2, ypxl2 + 1, color, fpart(yend) * xgap);
        }

        // Main loop
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
                int y = (int)floorf(intery);
                float frac = fpart(intery);

                if (steep) {
                        setPixel(renderer, y, x, color, 1.0f - frac);
                        setPixel(renderer, y + 1, x, color, frac);
                } else {
                        setPixel(renderer, x, y, color, 1.0f - frac);
                        setPixel(renderer, x, y + 1, color, frac);
                }

                intery += gradient;
        }
}

Point lerp(Point a, Point b, float t) {
        return (Point) {
                .x = a.x + (b.x - a.x) * t,
                .y = a.y + (b.y - a.y) * t,
        };
}

void renderBezierCurve(SDL_Renderer *renderer, Point p0, Point p1, Point p2, Point p3, Pan pan, int steps, SDL_Color color) {
        Point prev = p0;

        for (int i = 1; i <= steps; i++) {
                float t = i / (float)steps;

                Point a = lerp(p0, p1, t);
                Point b = lerp(p1, p2, t);
                Point c = lerp(p2, p3, t);

                Point d = lerp(a, b, t);
                Point e = lerp(b, c, t);

                Point f = lerp(d, e, t); // Final point on curve

                BetterLine(renderer, prev.x + pan.x, prev.y + pan.y, f.x + pan.x, f.y + pan.y, color);
                prev = f;
        }
}

int estimateSteps(Point p0, Point p1, Point p2, Point p3) {
        float maxDeviation = fmaxf(perpendicularDistance(p0, p3, p1), perpendicularDistance(p0, p3, p2));
        float length = hypotf(p3.x - p0.x, p3.y - p0.y);

        // Normalize deviation
        if (length < 1e-5f) {
                length = 1e-5f;
        }
        float deviationFactor = maxDeviation / length;

        // Clamp steps to sane values
        int steps = (int)(70 + deviationFactor * 200); // from 70 (flat) to 200 (curvy)
        if (steps < 70) steps = 70;
        if (steps > 200) steps = 200;

        return steps;
}


void __RenderLines__(SDL_Renderer* renderer, LinesArray *PA, Pan pan, uint16_t line_start_index, uint16_t line_end_index, SDL_Color color) {
        if (line_end_index == line_start_index) {
                return;
        }
        Point arr[4];
        int temp = 0;

        for (int i = line_start_index; i <= line_end_index; i++) {
                arr[temp] = PA->points[i];
                temp++;

                if (!PA->points[i].connected_to_next_point) {
                        // Handle disconnected segments
                        if (temp == 2) {
                                BetterLine(renderer, arr[0].x + pan.x, arr[0].y + pan.y, arr[1].x + pan.x, arr[1].y + pan.y, color);
                                SDL_SetRenderDrawColor(renderer, unpack_color(color));
                        } else if (temp == 3) {
                                int steps = estimateSteps(arr[0], arr[1], arr[2], arr[2]);
                                renderBezierCurve(renderer, arr[0], arr[1], arr[2], arr[2], pan, steps, color);
                        }

                        temp = 0;
                }

                if (temp == 4) {
                        int steps = estimateSteps(arr[0], arr[1], arr[2], arr[3]);
                        renderBezierCurve(renderer, arr[0], arr[1], arr[2], arr[3], pan, steps, color);
                        arr[0] = arr[3];
                        temp = 1;
                }
        }

        // Handle leftovers
        if (temp == 2) {
                BetterLine(renderer, arr[0].x + pan.x, arr[0].y + pan.y, arr[1].x + pan.x, arr[1].y + pan.y, color);
        } else if (temp == 3) {
                int steps = estimateSteps(arr[0], arr[1], arr[2], arr[2]);
                renderBezierCurve(renderer, arr[0], arr[1], arr[2], arr[2], pan, steps, color);
        } else if (temp == 4) {
                int steps = estimateSteps(arr[0], arr[1], arr[2], arr[3]);
                renderBezierCurve(renderer, arr[0], arr[1], arr[2], arr[3], pan, steps, color);
        }

        PA->rendered_till = PA->pointCount - 1;
}

void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA, Pan pan, SDL_Color color) {
        if (PA->pointCount != 0) {
                __RenderLines__(renderer, PA, pan, 0, PA->pointCount - 1, color);
        }
}

void RenderLine(SDL_Renderer* renderer, LinesArray* PA, Pan pan, uint16_t start_index, uint16_t end_index, SDL_Color color) {
        if (PA == NULL || PA->pointCount == 0){
                return;
        }

        SDL_SetRenderDrawColor(renderer, unpack_color(color));
        uint16_t rendered_till = start_index;
        while (rendered_till < end_index - 1) {
                if (PA->points[rendered_till].connected_to_next_point && PA->points[rendered_till + 1].connected_to_next_point) {
                        SDL_RenderDrawLine(renderer,
                                (int) (PA->points[rendered_till].x + pan.x),
                                (int) (PA->points[rendered_till].y + pan.y),
                                (int) (PA->points[rendered_till + 1].x + pan.x),
                                (int) (PA->points[rendered_till + 1].y + pan.y)
                        );
                }
                rendered_till += 1;
        }
}

void PanPoints(Pan* pan, float xrel, float yrel) {
        pan->x += xrel;
        pan->y += yrel;
}

void douglasPeucker(Point* points, int start, int end, double epsilon, bool* keep) {
        if (end <= start + 1) {
                keep[start] = true;
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

        if (maxDist >= epsilon) {
                keep[index] = true;
                douglasPeucker(points, start, index, epsilon, keep);
                douglasPeucker(points, index, end, epsilon, keep);
        }
}

float calculateEpsilon(Point* points, int count) {
        float total = 0;
        int valid = 0;

        for (int i = 1; i < count - 1; i++) {
                float d = perpendicularDistance(points[0], points[count - 1], points[i]);
                total += d;
                valid++;
        }

        float mean = (valid > 0) ? (total / valid) : 0;

        total = 0;
        for (int i = 1; i < count - 1; i++) {
                float d = perpendicularDistance(points[0], points[count - 1], points[i]);
                total += pow(d - mean, 2);
        }

        float stddev = sqrt(total / (count));
        return stddev * 2e-3f;
}


void OptimizeLine(LinesArray* PA, uint16_t line_start_index, uint16_t line_end_index) {
        if (PA->pointCount == 0) return;

        double epsilon = calculateEpsilon(PA->points, PA->pointCount);
        bool* keep = calloc(PA->pointCount, sizeof(int));
        douglasPeucker(PA->points, line_start_index, line_end_index, epsilon, keep);
        keep[line_start_index] = 1;
        keep[line_end_index] = 1;

        uint16_t temp = 0;
        for (int i = line_start_index; i <= line_end_index; ++i) {
                if (keep[i]) {
                        PA->points[line_start_index + temp] = PA->points[i];
                        temp += 1;
                }
        }

        PA->pointCount = line_start_index + temp;
        PA->rendered_till = PA->pointCount;
        free(keep);
}

#undef unwrap_color
#undef swap
#undef POINTS_THRESHOLD
#undef fpart
#undef rfpart
