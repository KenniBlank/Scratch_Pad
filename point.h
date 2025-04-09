#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#pragma once

typedef struct {
        int32_t x, y;
        uint8_t line_thickness;
        bool connected_to_previous_point;
} Point;

typedef struct {
        Point *points;  // Pointer to dynamic array of points
        uint16_t pointCount; // Current number of points
        uint16_t pointCapacity;      // Max capacity of the array
        SDL_Color color;        // Color of the Lines
} LinesArray;

void addPoint(LinesArray* PA, int32_t x, int32_t y, uint8_t line_thickness, bool connected_to_prev_line);
void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA);
