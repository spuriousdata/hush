CC=clang
CFLAGS=-Wall -Isrc/include $(shell pkg-config --cflags fuse libsodium) -std=c++1y -g -c
LDFLAGS=$(shell pkg-config --libs libsodium) $(shell pkg-config --libs fuse) -lstdc++

EXE=hush
CRYPTO=src/crypto/secretkey.o src/crypto/symmetric.o
UTILS=src/utils/optparse.o src/utils/password.o
ACTIONS=src/actions/keygen.o src/actions/mount.o src/actions/create.o

OBJS=src/main.o \
	 $(ACTIONS) \
	 $(UTILS) \
	 $(CRYPTO)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	
%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm $(OBJS) $(EXE)
