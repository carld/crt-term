
PLATFORM = $(shell uname)
REV = $(shell git rev-parse HEAD)

ifeq ($(findstring Linux,$(PLATFORM)),Linux)
  $(info Building revision $(REV) for $(PLATFORM))
  $(info Using libsdl2 version $(shell sdl2-config --version))
else
  $(error $(PLATFORM) is not supported)
  $(shell exit 2)
endif

GIT = /usr/bin/git

CC = gcc

CFLAGS += -g -Wall -O2  -D_GNU_SOURCE
LFLAGS += -lGLEW -lglfw -lGL -lGLU

SRC = main.c  render.c
OBJ = $(SRC:.c=.o)
BIN = app

.PHONY: clean

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ) 
	$(CC) -o $@ $(OBJ)  $(LFLAGS)

clean:
	rm -vf $(BIN) $(OBJ)


