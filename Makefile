CC=clang
CFLAGS=-Wall -Isrc/include $(shell pkg-config --cflags fuse libsodium) -std=c++1y
LDFLAGS=$(shell pkg-config --libs libsodium) $(shell pkg-config --libs fuse) -lstdc++ -ldl

# DEBUG
CFLAGS+=-g
# DEBUG

BIN=hush
TESTBIN=runtests
CRYPTO=src/crypto/secretkey.o src/crypto/symmetric.o 
UTILS=src/utils/optparse.o src/utils/password.o src/utils/tools.o
ACTIONS=src/actions/keygen.o src/actions/mount.o src/actions/create.o

OBJS=src/main.o \
	 $(ACTIONS) \
	 $(UTILS) \
	 $(CRYPTO)

TESTOBJS=src/test/main.o \
		 src/test/log.o \
		 src/test/b64.o

DEPS := $(OBJS:.o=.d) $(TESTOBJS:.o=.d)

all: $(BIN)
	
-include $(DEPS)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cc
	$(CC) $(CFLAGS) -MMD -MF $(<:.cc=.d) -c -o $@ $<

test: $(TESTBIN)
	./runtests
	
$(TESTBIN): $(TESTOBJS)
	$(CC) $(LDFLAGS) -o $@ $(TESTOBJS)
	
clean:
	-rm $(OBJS) $(BIN) $(TESTOBJS) $(TESTBIN)

distclean: clean
	-find . -type f -name \*.o -o -name \*.d | xargs rm
