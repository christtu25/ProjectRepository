CC = gcc
CFLAGS = -Wall
all: cooke_analyzer

cooke_analyzer: lexical_analyzer.c
	$(CC) $(CFLAGS) -o cooke_analyzer lexical_analyzer.c

clean:
	rm -f cooke_analyzer *.o