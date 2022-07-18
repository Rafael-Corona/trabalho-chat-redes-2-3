CC=gcc
MAIN=./src/main.c
BINARY=./prog
SOURCES=
INCLUDES=./lib
DEBUGGER=valgring

all:
	$(CC) -o $(BINARY) $(MAIN) $(SOURCES) -I $(INCLUDES) -Wall

run:
	$(BINARY)

debug:
	$(DEBUGGER) $(BINARY)
