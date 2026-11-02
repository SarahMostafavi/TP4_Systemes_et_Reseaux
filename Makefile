SRC = $(filter-out mysem.c, $(wildcard *.c))
BIN = $(patsubst %.c,%,$(SRC))

CFLAGS += -Wall -Wextra -g
#CFLAGS += --std=c99

LDLIBS += libseg.a

all: $(BIN) client_perso

$(BIN): segdef.h

client_perso: client.c segdef.h
	$(CC) $(CFLAGS) client.c -o $@

clean:
	rm -f $(BIN) client_perso
	rm -f *.o
	rm -f *~

indent:
	indent -linux -i4 -nut -ts2 *.c

.PHONY: all clean indent
