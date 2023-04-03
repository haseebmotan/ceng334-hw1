CC=gcc
CFLAGS=-g

all: bgame

bgame: main.c
	$(CC) $(CFLAGS) main.c message.c logging.c -o bgame

clean:
	rm bgame