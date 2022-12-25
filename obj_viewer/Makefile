.PHONY: clean all

# OS specific linker flags for SDL2
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
LDFLAGS=-L../glad/gles3.2/src -lGL -lGLU -lm -pthread -ldl `pkg-config --libs sdl2`
CFLAGS=-I../glad/gles3.2/include -Wall -Wextra -Wshadow -Wformat=2 -pedantic -std=c99 -O2 -g `pkg-config --cflags sdl2`
CC=gcc
else
LDFLAGS=-lm -pthread -ldl -lglu32 `pkg-config --libs sdl2`
CFLAGS=-I../glad/gl3.0a/include -Wformat=2 -std=c99 -O2 -g `pkg-config --cflags sdl2`
CC=gcc
endif

all: viewer

viewer: viewer.c
	$(CC) viewer.c trackball.c ../glad/gl3.0a/src/glad.c -o viewer $(LDFLAGS) $(CFLAGS)

clean:
	@rm -rf viewer
