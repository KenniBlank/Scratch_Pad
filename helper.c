#include "helper.h"
#include <SDL2/SDL_mouse.h>
#include <stdint.h>
#include <sys/types.h>

SDL_Cursor* createCursorFromPNG(const char* filename, uint8_t width, uint8_t height) {
        SDL_Surface* original = IMG_Load(filename);
        if (!original) {
                SDL_Log("Failed to load PNG: %s", IMG_GetError());
                return NULL;
        }

        // Create a surface with the desired size and same pixel format
        SDL_Surface* resized = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, original->format->format);
        if (!resized) {
                SDL_Log("Failed to create resized surface: %s", SDL_GetError());
                SDL_FreeSurface(original);
                return NULL;
        }

        // Scale original into resized
        if (SDL_BlitScaled(original, NULL, resized, NULL) != 0) {
                SDL_Log("Failed to scale surface: %s", SDL_GetError());
                SDL_FreeSurface(original);
                SDL_FreeSurface(resized);
                return NULL;
        }

        SDL_FreeSurface(original);

        SDL_Cursor* cursor = SDL_CreateColorCursor(resized, width / 2, height / 2);
        if (!cursor) {
                SDL_Log("Failed to create cursor: %s", SDL_GetError());
        }

        SDL_FreeSurface(resized);
        return cursor;
}

SDL_Texture* LoadImageAsTexture(const char* path, SDL_Renderer* renderer) {
        SDL_Surface* surface = IMG_Load(path);
        if (!surface) {
                printf("Image Load Error: %s\n", IMG_GetError());
                return NULL;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
}

char* unique_name(char* folderLocation, char* prefix) {
        // Calculate a safe length for returnValue (prefix + count + extension)
        size_t len_return_value = strlen(folderLocation) + strlen(prefix) + 10;

        char* returnValue = malloc(sizeof(char) * len_return_value);
        if (!returnValue) {
                __DEBUG__("Memory allocation failed");
                return NULL;
        }

        // Ensure the folder exists
        struct stat st = {0};
        if (stat(folderLocation, &st) == -1) {
                __DEBUG__("Folder doesn't exist");
                free(returnValue);
                return NULL;
        }

        // Get the number of files in the folder
        char command[256];
        snprintf(command, sizeof(command), "ls \"%s\" | wc -l", folderLocation);
        FILE *fp = popen(command, "r");
        if (!fp) {
                __DEBUG__("Failed to run command to get file count");
                free(returnValue);
                return NULL;
        }

        int count = 0;
        if (fscanf(fp, "%d", &count) != 1) {
                __DEBUG__("Failed to read file count");
                pclose(fp);
                free(returnValue);
                return NULL;
        }
        pclose(fp);

        // Generate a unique file name
        while (1) {
                snprintf(returnValue, len_return_value, "%s%s%05d.png", folderLocation, prefix, count);
                snprintf(command, sizeof(command), "ls \"%s\" | grep \"%s\" | wc -l", folderLocation, returnValue);

                fp = popen(command, "r");
                if (!fp) {
                        __DEBUG__("Failed to run command to check uniqueness");
                        free(returnValue);
                        return NULL;
                }

                int fileCount = 0;
                if (fscanf(fp, "%d", &fileCount) != 1) {
                        __DEBUG__("Failed to read the uniqueness check result");
                        pclose(fp);
                        free(returnValue);
                        return NULL;
                }
                pclose(fp);

                if (fileCount == 0) {
                        return returnValue;
                }
                count++;
        }
}

void SaveRendererAsImage(SDL_Renderer *renderer, char *Suffix, char *Location) {
        int win_width, win_height;
        SDL_GetRendererOutputSize(renderer, &win_width, &win_height);

        SDL_Surface *surface = SDL_CreateRGBSurface(0, win_width, win_height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!surface) {
                printf("Unable to create surface: %s\n", SDL_GetError());
                return;
        }

        if (!surface -> pixels) {
                printf("Surface pixels are NULL after creation\n");
                SDL_FreeSurface(surface);
                surface = NULL;
                return;
        }

        char *file_name = unique_name(Location, Suffix);
        if (file_name == NULL) {
                printf("Error generating filename..\n");
                SDL_FreeSurface(surface);
                return;
        }

        if (SDL_RenderReadPixels(renderer, NULL, surface -> format -> format, surface -> pixels, surface -> pitch) < 0) {
                printf("Unable to read pixels: %s\n", SDL_GetError());
                free(file_name);
                SDL_FreeSurface(surface);
                return;
        }

        if (IMG_SavePNG(surface, file_name) != 0) {
                printf("Unable to save frame as PNG: %s\n", IMG_GetError());
                free(file_name);
                return;
        }
        SDL_FreeSurface(surface);
        free(file_name);
}

void print_live_usage() {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        printf("[Live] CPU time: %ld.%06lds | Memory (maxrss): %ld KB\n",
                usage.ru_utime.tv_sec,
                usage.ru_utime.tv_usec,
                usage.ru_maxrss
        );

        fflush(stdout);
}

bool CollisionDetection(uint16_t x1, uint16_t y1, uint16_t w1, uint16_t h1, uint16_t x2, uint16_t y2, uint16_t w2, uint16_t h2) {
        return x1 < (x2 + w2) && (x1 + w1) > x2 && y1 < y2 + h2 && (y1 + h1) > y2;
}
