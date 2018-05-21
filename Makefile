
CC = gcc
CFLAGS = -Wall -std=c99

cdc_example: example.o
	$(CC) -o example example.o -lusb-1.0

default: example

clean:
	rm example example.o
