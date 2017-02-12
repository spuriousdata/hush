CC=clang
CFLAGS=-Wall -Isrc/include $(shell pkg-config --cflags fuse libsodium) -std=c++1y -g -c
LDFLAGS=$(shell pkg-config --libs libsodium) $(shell pkg-config --libs fuse) -lstdc++

EXE=hush
UTILS=src/utils/optparse.o
ACTIONS=src/actions/keygen.o src/actions/mount.o src/actions/create.o

OBJS=src/main.o \
	 $(ACTIONS) \
	 $(UTILS)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	
%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm $(OBJS) $(EXE)
