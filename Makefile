# Makefile — Smart Waste Management System v3
# Usage:
#   make          → builds the executable
#   make clean    → removes object files and executable
#   make run      → builds and runs

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude
LDFLAGS = -lm

SRCS = src/globals.c \
       src/utils.c   \
       src/bins.c    \
       src/fleet.c   \
       src/operations.c \
       src/reports.c \
       src/civilian.c \
       src/main.c

OBJS = $(SRCS:.c=.o)
TARGET = waste_mgmt

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean
