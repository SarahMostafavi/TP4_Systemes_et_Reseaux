SRC = $(filter-out mysem.c, $(wildcard *.c))
BIN = $(patsubst %.c,%,$(SRC))

CFLAGS += -Wall -Wextra -g
#CFLAGS += --std=c99

LDLIBS += libseg.a

all: $(BIN)

# Chaque programme est compilé depuis son .c, avec la bibliothèque fournie
$(BIN): %: %.c segdef.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Partie 3 : client avec nos propres routines de sémaphores (mysem.c)
client_perso: client.c mysem.c segdef.h
	$(CC) $(CFLAGS) client.c mysem.c -o $@

clean:
	rm -f $(BIN) client_perso
	rm -f *.o
	rm -f *~

indent:
	indent -linux -i4 -nut -ts2 *.c

.PHONY: all clean indent
