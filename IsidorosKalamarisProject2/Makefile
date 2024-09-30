CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude #also added flag for threads and include path
BINDIR = bin
SRCDIR = src

all: $(BINDIR)/jobExecutorServer $(BINDIR)/jobCommander

$(BINDIR)/jobExecutorServer: $(SRCDIR)/jobExecutorServer.c
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/jobCommander: $(SRCDIR)/jobCommander.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(BINDIR)/jobExecutorServer $(BINDIR)/jobCommander
