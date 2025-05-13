CC = gcc
CFLAGS = -Wall

all: cooke_parser

cooke_parser: parser.c
	$(CC) $(CFLAGS) -o cooke_parser parser.c

clean:
	rm -f cooke_parser *.o