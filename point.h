#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#pragma once

typedef struct {
       float x, y;
} Pan;

typedef struct {
        int32_t x, y;
        uint8_t line_thickness;
        bool connected_to_next_point;
} Point;

typedef struct {
        Point *points;  // Pointer to dynamic array of points
        uint16_t pointCount; // Current number of points
        uint16_t pointCapacity;      // Max capacity of the array
        uint16_t rendered_till;
} LinesArray;

int addPoint(LinesArray* PA, int32_t x, int32_t y, uint8_t line_thickness, bool connected_to_prev_line);
void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA, Pan pan, SDL_Color color);
void RenderLines(SDL_Renderer* renderer, LinesArray* PA, Pan pan, SDL_Color color);
void PanPoints(Pan* pan, float xrel, float yrel);
void OptimizeLine(LinesArray* PA, uint16_t line_start_index, uint16_t line_end_index);
