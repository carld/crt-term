
PLATFORM = $(shell uname)
REV = $(shell git rev-parse HEAD)

$(info Building revision $(REV) for $(PLATFORM))

CMAKE_OPTS = -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_DOCS=OFF \
             -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
ifeq (Linux,$(PLATFORM))
	CC = gcc
	CFLAGS += -DLINUX=1
	LFLAGS += -lGLU -lGL -lGLEW
	LFLAGS += $(GLFW) -lrt -lm -ldl -lX11 -lpthread -lXrandr -lXinerama -lXxf86vm -lXcursor -lXi
	GLFW_TAG = 3.0.4
endif

ifeq (Darwin,$(PLATFORM))
	CC = cc
	CFLAGS += -DDARWIN=1
	LFLAGS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework Carbon
	CMAKE_OPTS += -DGLFW_USE_RETINA=OFF -DGLFW_USE_CHDIR=OFF
	PATCH_LIBSHL = patch -d libshl -p 1 < libshl_osx.patch
	GLFW_TAG = 3.0.4
endif

GIT = $(which git)

GLFW = glfw/src/libglfw3.a

CFLAGS += -g -ggdb -Wall -D_GNU_SOURCE
CFLAGS += -I./libtsm/src -I./libtsm -I./libshl/src
CFLAGS += -I./glfw/include
CFLAGS += -I./libxkbcommon
CFLAGS += -pg
LFLAGS += -pg
LFLAGS += $(GLFW)

TSM = $(wildcard libtsm/src/*.c) $(wildcard libtsm/external/*.c)
SHL = libshl/src/shl_pty.c

SRC = $(SHL) $(TSM)
SRC += libxkbcommon/src/keysym-utf.c libxkbcommon/src/utf8.c
SRC += $(wildcard src/*.c)

OBJ = $(SRC:.c=.o)
BIN = crt-term

.PHONY: clean prebuild $(BIN).app

all: prebuild $(BIN)

prebuild:
	$(PATCH_LIBSHL); true

.c.o:  $(SRC)
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(GLFW) $(OBJ)
	$(CC) -o $@ $(OBJ)  $(LFLAGS)
	./$(BIN) -h

$(GLFW):
	cd glfw; \
	git checkout $(GLFW_TAG); \
	cmake $(CMAKE_OPTS) .; \
	make

clean:
	cd glfw; make clean
	rm -vf $(BIN) $(OBJ)
	rm -rf $(BIN).app

total_clean: clean
	cd libshl && git checkout . && git clean -f && cd ..
	cd glfw && git checkout . && git clean -f && cd ..

$(BIN).app:
	mkdir -p $@/Contents/MacOS
	mkdir -p $@/Contents/Resources
	cp $(BIN) $@/Contents/MacOS
	cp fonts/* $@/Contents/MacOS
	cp shaders/* $@/Contents/MacOS
	cp Info.plist $@/Contents


