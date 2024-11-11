CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -pthread -lrt

SRCS = main.c buffer.c app.c
OBJS = $(SRCS:.c=.o)

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main

.PHONY: all clean
