CC=clang
CFLAGS=-Wall -Isrc/include $(shell pkg-config --cflags fuse libsodium) -std=c++1y -g -DLOGURU_DEBUG_LOGGING
LDFLAGS=$(shell pkg-config --libs libsodium) $(shell pkg-config --libs fuse) -lstdc++ -ldl

BIN=hush
TESTS=b64test
CRYPTO=src/crypto/secretkey.o src/crypto/symmetric.o 
UTILS=src/utils/optparse.o src/utils/password.o src/utils/tools.o src/utils/loguru.o
ACTIONS=src/actions/keygen.o src/actions/mount.o src/actions/create.o

OBJS=src/main.o \
	 $(ACTIONS) \
	 $(UTILS) \
	 $(CRYPTO)

DEPS := $(OBJS:.o=.d)

all: $(BIN)
	
-include $(DEPS)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cc
	$(CC) $(CFLAGS) -MMD -MF $(<:.cc=.d) -c -o $@ $<

test: $(TESTS)
	
b64test: src/test/b64test.cc src/include/utils/b64.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	
clean:
	-rm $(OBJS) $(BIN) $(TESTS)
