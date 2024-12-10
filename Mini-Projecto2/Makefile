CC = gcc
CFLAGS = -Wall -Werror -O2 -g
LDFLAGS = 

SOURCES = file-sender.c file-receiver.c
HEADERS = packet-format.h
EXECUTABLES = file-sender file-receiver

.PHONY: all clean debug test

all: $(EXECUTABLES)

debug: CFLAGS += -DDEBUG -g
debug: all

file-sender: file-sender.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

file-receiver: file-receiver.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

log-packets.so: log-packets.c
	$(CC) -shared -fPIC $(CFLAGS) -o $@ $< -ldl

test: $(EXECUTABLES)
	./test-submission2.sh

clean:
	rm -f $(EXECUTABLES) *.log *.so