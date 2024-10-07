CC=g++

MAKEFLAGS := --jobs=$(shell nproc)

LDFLAGS=-L/usr/X11/lib -lX11 -lXext -lXfixes

OBJ_FILES=xdvd.cpp.o
OBJS=$(addprefix obj/, $(OBJ_FILES))
BIN=xdvd

.PHONY: all clean

#-------------------------------------------------------------------------------

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
