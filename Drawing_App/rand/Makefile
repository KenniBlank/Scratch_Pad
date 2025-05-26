CC = gcc

CFLAGS = -std=gnu2x -Wall -Wextra -Wshadow
LIBS = -lSDL2 -lm -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lcairo

# CFILES = point.c

DEBUGFLAGS = -g -DDEBUG
#-Werror

all:
	@echo "Usage: make <program_name> (without .c extension)"

%:
	$(CC) $@.c -o $@ `pkg-config --cflags --libs gtk+-3.0` $(CFLAGS) $(LIBS)
	./$@
	rm $@
