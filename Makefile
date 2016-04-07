
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

XKB = libxkbcommon/.libs/libxkbcommon.a
GLFW = glfw/src/libglfw3.a

CFLAGS += -g -ggdb -Wall -D_GNU_SOURCE
CFLAGS += -I./libtsm/src -I./libtsm -I./libshl/src
CFLAGS += -I./glfw/include
CFLAGS += -I./libxkbcommon
LFLAGS += -lGLU -lGL -lGLEW -lm -lX11 
LFLAGS += $(GLFW) -lrt -lm -ldl -lX11 -lpthread -lXrandr -lXinerama -lXxf86vm -lXcursor
LFLAGS += $(XKB)

TSM = $(wildcard libtsm/src/*.c) $(wildcard libtsm/external/*.c)
SHL = libshl/src/shl_pty.c

SRC = $(SHL) $(TSM)
SRC += main.c display.c font/bdf.c terminal.c 
SRC += shader.c select_array.c
OBJ = $(SRC:.c=.o)
BIN = crt-term

.PHONY: clean

.c.o:  $(SRC)
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(XKB) $(GLFW) $(OBJ)
	$(CC) -o $@ $(OBJ)  $(LFLAGS)

$(XKB):
	cd libxkbcommon; \
        ./autogen.sh; \
        ./configure; \
        make

$(GLFW):
	cd glfw; \
	cmake -DBUILD_SHARED_LIBS=ON \
              -DBUILD_STATIC_LIBS=ON \
              -DGLFW_BUILD_DOCS=OFF \
              -DGLFW_BUILD_EXAMPLES=OFF \
              -DGLFW_BUILD_TESTS=OFF .; \
	make

clean:
	cd glfw; make clean
	cd libxkbcommon; make clean
	rm -vf $(BIN) $(OBJ)


