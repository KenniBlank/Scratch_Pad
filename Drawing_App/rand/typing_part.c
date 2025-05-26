#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define unpack_color(color) color.r, color.g, color.b, color.a

#define ret_success 0
#define ret_failure -1
#define ret_mem_error -2

int G_border_radius = 12;
int G_font_size = 20;

typedef struct {
        char *txt;
        int capacity;
        int length;
} String;

typedef struct {
        String txt;
        SDL_Rect size;
} PostIt;

struct double_int {
        // Returns two integer value
        int a, b;
};

typedef struct {
      PostIt** postits;

      SDL_Color bg_color;
      SDL_Color txt_color;
      SDL_Color border_color;

      int capacity;
      int count;

      TTF_Font** font;

      int min_width;
      int min_height;

      bool isDragging;

      int drag_offset_x;
      int drag_offset_y;

      int currently_usr_selected_post_it; // -1 is non-selected case
} PostIts;

int update_post_it_data(PostIts* post_it_arr, char ch);

int mouseInRect(SDL_Rect rect, int x, int y) {
        return x >= rect.x && x <= (rect.x + rect.w) && y >= rect.y && y <= (rect.y + rect.h);
}

int add_postit(PostIts* post_its_arr, PostIt* newPostIt) {
        if (newPostIt == NULL || post_its_arr == NULL) {
                return ret_failure;
        }

        if (post_its_arr->capacity <= post_its_arr->count) {
                int new_capacity = post_its_arr->capacity == 0 ? 1 : post_its_arr->capacity * 2;
                PostIt** new_array = realloc(post_its_arr->postits, new_capacity * sizeof(PostIt*));
                if (new_array == NULL) {
                        return ret_mem_error;
                }
                post_its_arr->postits = new_array;
                post_its_arr->capacity = new_capacity;
        }

        post_its_arr->postits[post_its_arr->count] = newPostIt;
        post_its_arr->count += 1;

        post_its_arr->currently_usr_selected_post_it = post_its_arr->count - 1;

        return ret_success;
}

int render_txt(SDL_Renderer* renderer, TTF_Font* font, SDL_Rect* rect, int min_height, const char* txt, const SDL_Color txt_color) {
        SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, txt, txt_color, rect->w - G_font_size);
        if (!surface) {
                return ret_mem_error;
        }

        // For dynamic height change:
        rect->h = SDL_max(surface->h + G_font_size, min_height);

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
                return ret_mem_error;
        }

        SDL_Rect dst = {
                rect->x + G_font_size / 2,
                rect->y + G_font_size / 2,
                surface->w,
                surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        return ret_success;
}

void pop_char(char *str) {
        if (str != NULL && *str != '\0') {
                str[strlen(str) - 1] = '\0';
        }
}

char* replace(const char* str, const char* old_substr, const char* new_substr) {
        if (str == NULL || old_substr == NULL || new_substr == NULL) {
                return "";
        }

        size_t str_len = strlen(str);
        size_t old_substr_len = strlen(old_substr);
        size_t new_substr_len = strlen(new_substr);

        size_t count = 0;
        const char* tmp = str;
        while ((tmp = strstr(tmp, old_substr))) {
                count++;
                tmp += old_substr_len;
        }

        size_t new_len = str_len + count * (new_substr_len - old_substr_len);

        char* result = (char*)malloc(new_len + 1);
        if (result == NULL) {
                return NULL;
        }

        char* current_pos = result;
        const char* start = str;
        while (count--) {
                const char* found = strstr(start, old_substr);
                size_t segment_len = (found >= start) ? (size_t)(found - start) : 0;

                memcpy(current_pos, start, segment_len);
                current_pos += segment_len;

                memcpy(current_pos, new_substr, new_substr_len);
                current_pos += new_substr_len;

                start = found + old_substr_len;
        }

        strcpy(current_pos, start);

        return result;
}

bool blinker_toggle_state() {
        static bool state = false;
        static Uint32 last_toggle = 0;
        Uint32 now = SDL_GetTicks(); // Get time in milliseconds

        if (now - last_toggle >= 700) { // If 700 milli second has passed
                state = !state;
                last_toggle = now;
        }
        return state;
}

char* format_txt(char *str) {
        char* formatted_txt = replace(str, "\t", "        ");
        return formatted_txt;
}

void RenderPostIts(SDL_Renderer* renderer, PostIts* post_it_arr) {
        for (int i = 0; i < post_it_arr->count; i++) {
                // If Selected, Add a Shadow effect
                if (i == post_it_arr->currently_usr_selected_post_it) {
                        static const SDL_Color shadow_color = {
                                .r = 35,
                                .g = 35,
                                .b = 35,
                                .a = 35
                        };
                        const int factor = G_font_size / 2;
                        roundedBoxRGBA(
                                renderer,
                                post_it_arr->postits[i]->size.x - factor,
                                post_it_arr->postits[i]->size.y + factor,
                                post_it_arr->postits[i]->size.x + post_it_arr->postits[i]->size.w - factor,
                                post_it_arr->postits[i]->size.y + post_it_arr->postits[i]->size.h + factor,
                                G_border_radius,
                                unpack_color(shadow_color)
                        );
                }

                // Background and Border Logic
                roundedBoxRGBA(
                        renderer,
                        post_it_arr->postits[i]->size.x,
                        post_it_arr->postits[i]->size.y,
                        post_it_arr->postits[i]->size.x + post_it_arr->postits[i]->size.w,
                        post_it_arr->postits[i]->size.y + post_it_arr->postits[i]->size.h,
                        G_border_radius,
                        unpack_color(post_it_arr->bg_color)
                );
                roundedRectangleRGBA(
                        renderer,
                        post_it_arr->postits[i]->size.x,
                        post_it_arr->postits[i]->size.y,
                        post_it_arr->postits[i]->size.x + post_it_arr->postits[i]->size.w,
                        post_it_arr->postits[i]->size.y + post_it_arr->postits[i]->size.h,
                        G_border_radius,
                        unpack_color(post_it_arr->border_color)
                );

                if (i == post_it_arr->currently_usr_selected_post_it) {
                        roundedRectangleRGBA(
                                renderer,
                                post_it_arr->postits[i]->size.x + 1,
                                post_it_arr->postits[i]->size.y + 1,
                                post_it_arr->postits[i]->size.x + post_it_arr->postits[i]->size.w - 1,
                                post_it_arr->postits[i]->size.y + post_it_arr->postits[i]->size.h - 1,
                                G_border_radius,
                                0, 255, 255, 255
                        );
                } else {
                        roundedRectangleRGBA(
                                renderer,
                                post_it_arr->postits[i]->size.x + 1,
                                post_it_arr->postits[i]->size.y + 1,
                                post_it_arr->postits[i]->size.x + post_it_arr->postits[i]->size.w - 1,
                                post_it_arr->postits[i]->size.y + post_it_arr->postits[i]->size.h - 1,
                                G_border_radius,
                                255, 255, 255, 255
                        );
                }

                // Formatting
                bool blinker = blinker_toggle_state();
                if (blinker && post_it_arr->currently_usr_selected_post_it == i) {
                        update_post_it_data(post_it_arr, '_');
                }
                const char* formatted_txt = format_txt(post_it_arr->postits[i]->txt.txt);
                if (blinker && post_it_arr->currently_usr_selected_post_it == i) {
                        pop_char(post_it_arr->postits[i]->txt.txt);
                }

                // Text Contain:
                render_txt(
                        renderer,
                        *post_it_arr->font,
                        &post_it_arr->postits[i]->size,
                        post_it_arr->min_height,
                        formatted_txt,
                        post_it_arr->txt_color
                );
        }
};

struct double_int longest_sentence(char* txt) {
        struct double_int return_value = { .a = 0, .b = 0 };

        if (txt == NULL || *txt == '\0') {
                return return_value;
        }

        int str_len = strlen(txt);
        int start_index = 0;

        for (int i = 0; i <= str_len; i++) {
                if (i == str_len || txt[i] == '\n') {
                        int current_length = i - start_index;
                        int max_length = return_value.b - return_value.a;

                        if (current_length > max_length) {
                                return_value.a = start_index;
                                return_value.b = i;
                        }
                        start_index = i + 1;
                }
        }

        return return_value;
}

char* str_slice(char* str, int start_index, int end_index) {
        if (str == NULL || start_index < 0 || end_index < start_index) {
                return NULL;
        }

        int length = end_index - start_index;
        char* return_value = malloc(length + 1); // +1 for null terminator
        if (return_value == NULL) {
                return NULL;
        }

        int j = 0;
        for (int i = start_index; i < end_index && str[i] != '\0'; i++) {
                return_value[j++] = str[i];
        }
        return_value[j] = '\0';

        return return_value;
}

int update_post_it_data(PostIts* post_it_arr, char ch) {
        if (post_it_arr->currently_usr_selected_post_it == -1) {
                return ret_success;
        }

        if (post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.length + 1 >= post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.capacity) {
                post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.capacity = post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.capacity == 0 ? 2: post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.capacity * 2;
                char *temp = realloc(post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.txt, post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.capacity);
                if (temp) {
                        post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.txt = temp;
                } else {
                        return ret_failure;
                }
        }

        char tmp[2] = { ch, '\0' };
        strcat(post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.txt, tmp);
        post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.length += 1;

        // Extend Rect if can't fit the line
        int txt_width = 0;
        struct double_int lng_sentence_index = longest_sentence(post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.txt);

        char* longest_sentence = str_slice(post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->txt.txt, lng_sentence_index.a, lng_sentence_index.b);
        TTF_SizeText(*post_it_arr->font, longest_sentence, &txt_width, NULL);
        free(longest_sentence);

        int new_width = SDL_max(post_it_arr->min_width, txt_width + G_font_size * 2);

        post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->size.w = new_width;
        return ret_success;
}

String post_it_txt(char* str) {
        String ret_str = {0};
        if (str != NULL && *str != '\0') {
                int len = strlen(str);
                ret_str.txt = malloc(sizeof(char) * (len + 1));
                if (ret_str.txt == NULL) {
                        return ret_str;
                }
                strcpy(ret_str.txt, str);
                ret_str.length = len;
                ret_str.capacity = len + 1;
        }
        return ret_str;
}

PostIt* newPostIt(PostIts* post_it_arr, int x, int y) {
        PostIt* new = malloc(sizeof(PostIt));

        new->size.x = x;
        new->size.y = y;
        new->size.w = post_it_arr->min_width;
        new->size.h = post_it_arr->min_height;

        new->txt = post_it_txt(">");
        post_it_arr->currently_usr_selected_post_it = post_it_arr->count - 1;
        return new;
}

void select_post_it(PostIts* post_it_arr, SDL_Event event) {
        int selected = -1;

        // First check if any post-it was clicked
        for (int i = 0; i < post_it_arr->count; i++) {
                if (mouseInRect(post_it_arr->postits[i]->size, event.button.x, event.button.y)) {
                        selected = i;
                        break;
                }
        }

        // If no post it selected, return
        if (selected == -1) {
                post_it_arr->currently_usr_selected_post_it = -1;
                return;
        }

        post_it_arr->currently_usr_selected_post_it = selected;
        post_it_arr->isDragging = true;
        post_it_arr->drag_offset_x = event.button.x - post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->size.x;
        post_it_arr->drag_offset_y = event.button.y - post_it_arr->postits[post_it_arr->currently_usr_selected_post_it]->size.y;
}
int main(void) {
        const int window_width = 1366, window_height = 768;

        SDL_Init(SDL_INIT_EVERYTHING);
        TTF_Init();

        SDL_Window *window = SDL_CreateWindow("Typing Logic", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_SHOWN);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        TTF_Font *font = TTF_OpenFont("ComingSoon_bold.ttf", G_font_size);
        int font_size = G_font_size; // For future change!

        SDL_StartTextInput(); // Enable text input

        SDL_Event event;
        bool app_running = true;

        SDL_Color bg_color = {
                .r = 255,
                .g = 255,
                .b = 255,
                .a = 255
        };

        PostIts post_its = {
                .postits = NULL,

                .capacity = 0,
                .count = 0,

                .bg_color = (SDL_Color) {.r = 255, .g = 184, .b = 107, .a = 255},
                // .bg_color = (SDL_Color) {.r = 100, .g = 100, .b = 100, .a = 255},
                .border_color = (SDL_Color) {.r = 0, .g = 0, .b = 0, .a = 255},
                .txt_color = (SDL_Color) {.r = 35, .g = 40, .b = 51, .a = 255},

                .font = &font,

                .min_height = 150,
                .min_width = 150,

                .isDragging = false,

                .drag_offset_x = 0,
                .drag_offset_y = 0,

                .currently_usr_selected_post_it = -1
        };

        while (app_running) {
                while(SDL_PollEvent(&event)) {
                        switch (event.type) {
                                case SDL_QUIT:
                                        app_running = false;
                                        break;

                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym ) {
                                                case SDLK_KP_ENTER:
                                                case SDLK_RETURN:
                                                        update_post_it_data(&post_its, '\n');
                                                        break;

                                                case SDLK_BACKSPACE:
                                                        if (post_its.currently_usr_selected_post_it != -1) {
                                                                pop_char(post_its.postits[post_its.currently_usr_selected_post_it]->txt.txt);
                                                                break;
                                                        }
                                                        break;

                                                case SDLK_TAB:
                                                case SDLK_KP_TAB:
                                                        update_post_it_data(&post_its, '\t');
                                                        break;

                                                case SDLK_INSERT:
                                                        add_postit(&post_its, newPostIt(&post_its, 100, 100));
                                                        break;

                                                default:
                                                        break;
                                        }
                                        break;

                                case SDL_MOUSEBUTTONDOWN:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        select_post_it(&post_its, event);
                                                        break;
                                                default: break;
                                        }
                                        break;

                                case SDL_MOUSEMOTION:
                                        if (post_its.isDragging) {
                                                post_its.postits[post_its.currently_usr_selected_post_it]->size.x = event.motion.x - post_its.drag_offset_x;
                                                post_its.postits[post_its.currently_usr_selected_post_it]->size.y = event.motion.y - post_its.drag_offset_y;
                                        }
                                        break;

                                case SDL_MOUSEBUTTONUP:
                                        switch (event.button.button) {
                                                case SDL_BUTTON_LEFT:
                                                        post_its.isDragging = false;
                                                        break;
                                                default: break;
                                        }
                                        break;

                                case SDL_TEXTINPUT:
                                        update_post_it_data(&post_its, *event.text.text);
                                        break;

                                default:
                                        break;
                        }
                }

                if (font_size != G_font_size) {
                        TTF_Font* new = TTF_OpenFont("ComingSoon_bold.ttf", G_font_size);
                        if (new) {
                                TTF_CloseFont(font);
                                font = new;
                        }
                        font_size = G_font_size;
                }

                SDL_SetRenderDrawColor(renderer, unpack_color(bg_color));
                SDL_RenderClear(renderer);

                RenderPostIts(renderer, &post_its);
                SDL_RenderPresent(renderer);

                SDL_Delay(16); // ms
        }

        if (post_its.postits != NULL) {
                for (int i = 0; i < post_its.count; i++) {
                        if (post_its.postits[i]->txt.txt != NULL) {
                                free(post_its.postits[i]->txt.txt);
                                free(post_its.postits[i]);
                        }
                }
                free(post_its.postits);
        }

        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 0;
}
