CC = gcc

CFLAGS = -std=gnu2x -Wall -Wextra -Wshadow
LIBS = -lSDL2 -lSDL2_image -lm -lSDL2_ttf

DEBUGFLAGS = -g -DDEBUG -Werror
RELEASEFLAGS = -O2 -DRELEASE

CFiles = App.c point.c helper.c
App = "App"

ifeq ($(build), RELEASE)
	CFLAGS += $(RELEASEFLAGS)
else
	CFLAGS += $(DEBUGFLAGS)
endif


all: compile

compile:
	$(CC) $(CFiles) -o $(App) $(CFLAGS) $(LIBS)

run: compile
	./$(App)
	@echo -e "\nProgram Return Value: $$?"

move: compile
	@mv ./$(App) ~
	@echo -e "Successfully moved file to Home"

clean:
	@rm $(App)
