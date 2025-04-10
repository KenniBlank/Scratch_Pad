CC = gcc

CFLAGS = -std=gnu2x -Wall -Wextra -Wshadow
DEBUGFLAGS = -g -DRELEASE -Werror
RELEASEFLAGS = -O2

LIBS = -lSDL2 -lSDL2_image -lm -lSDL2_ttf -lGL

CFiles = App.c point.c helper.c
App = "App"

CFLAGS += $(DEBUGFLAGS)
# CFLAGS += $(RELEASEFLAGS)

all: run

compile:
	$(CC) $(CFiles) -o $(App) $(CFLAGS) $(LIBS)

run: compile
	./$(App)
	@echo -e "\nProgram Return Value: $$?"

clean:
	@rm $(App)
