#include <SDL2/SDL.h>
#include <sys/resource.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wchar.h>

#pragma once
void print_live_usage();
void SaveRendererAsImage(SDL_Renderer *renderer, char *Suffix, char *Location);
bool CollisionDetection(uint16_t x1, uint16_t y1, uint16_t w1, uint16_t h1, uint16_t x2, uint16_t y2, uint16_t w2, uint16_t h2);
