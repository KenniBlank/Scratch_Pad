#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#pragma once

typedef struct {
       double x, y;
} Pan;

typedef struct {
        float x, y;
        uint8_t line_thickness;
        bool connected_to_next_point;
} Point;

typedef struct {
        Point *points;  // Pointer to dynamic array of points
        uint16_t pointCount; // Current number of points
        uint16_t pointCapacity;      // Max capacity of the array
        uint16_t rendered_till;
} LinesArray;

void PanPoints(Pan* pan, float xrel, float yrel);
void set_window_dimensions(int win_width, int win_height);
void ReRenderLines(SDL_Renderer* renderer, LinesArray *PA, Pan pan, SDL_Color color);
void OptimizeLine(LinesArray* PA, uint16_t line_start_index, uint16_t line_end_index);
int addPoint(LinesArray* PA, float x, float y, uint8_t line_thickness, bool connected_to_prev_line);
void RenderLine(SDL_Renderer* renderer, LinesArray* PA, Pan pan, uint16_t start_index, uint16_t end_index, SDL_Color color);
void __RenderLines__(SDL_Renderer* renderer, LinesArray *PA, Pan pan, uint16_t line_start_index, uint16_t line_end_index, SDL_Color color);
