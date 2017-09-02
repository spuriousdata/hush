CC=clang
CFLAGS=-Wall -Isrc/include $(shell pkg-config --cflags fuse libsodium) -std=c++1y
LDFLAGS=$(shell pkg-config --libs libsodium) $(shell pkg-config --libs fuse) -lstdc++ -ldl

# DEBUG
CFLAGS+=-g -DLOGURU_DEBUG_LOGGING
# DEBUG

BIN=hush
TESTS=b64test testlog vartest
CRYPTO=src/crypto/secretkey.o src/crypto/symmetric.o 
UTILS=src/utils/optparse.o src/utils/password.o src/utils/tools.o
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
	
b64test: src/test/b64test.cc
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

logtest: src/test/logtest.cc
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	
vartest: src/test/vartest.cc
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	
clean:
	-rm $(OBJS) $(BIN) $(TESTS)

distclean: clean
	-find . -type f -name \*.o -o -name \*.d | xargs rm
