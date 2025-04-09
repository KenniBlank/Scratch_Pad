#include <SDL2/SDL.h>
#include <sys/resource.h>
#include <SDL2/SDL_image.h>

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
