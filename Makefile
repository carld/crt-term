
PLATFORM = $(shell uname)
REV = $(shell git rev-parse HEAD)

ifeq ($(findstring Linux,$(PLATFORM)),Linux)
  $(info Building revision $(REV) for $(PLATFORM))
else
  $(error $(PLATFORM) is not supported)
  $(shell exit 2)
endif

GIT = /usr/bin/git

CC = gcc

CFLAGS += -g -ggdb -Wall -D_GNU_SOURCE
CFLAGS += -I./libtsm/src -I./libtsm -I./libshl/src
CFLAGS += -I./glfw/include
LFLAGS += -lGLU -lGL -lGLEW -lm  -lxkbcommon -lX11 
LFLAGS += ./glfw/src/libglfw3.a  -lrt -lm -ldl -lX11 -lpthread -lXrandr -lXinerama -lXxf86vm -lXcursor

TSM = $(wildcard libtsm/src/*.c) $(wildcard libtsm/external/*.c)
SHL = libshl/src/shl_pty.c

SRC = $(SHL) $(TSM)
SRC += main.c display.c font/bdf.c terminal.c 
SRC += shader.c
OBJ = $(SRC:.c=.o)
BIN = crt-term

.PHONY: clean glfw

.c.o:  $(SRC)
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ) glfw
	$(CC) -o $@ $(OBJ)  $(LFLAGS)

glfw:
	cd glfw; \
	cmake BUILD_SHARED_LIBS=true .; \
	make

clean:
	rm -vf $(BIN) $(OBJ)
	cd glfw; make clean


