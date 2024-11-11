# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -pthread -lrt

# Target executable name
TARGET = main

# Source files
SRCS = main.c buffer.c app.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Linking the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compiling source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove object files and the executable
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets to prevent conflicts with files of the same name
.PHONY: all cleanhttps://www.onlinegdb.com/#tab-stdin
