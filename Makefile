CC = cc
CFLAGS = -g -Wall -Wextra

OBJS = ncc.o token.o parser.o error.o

ncc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

ncc.o: ncc.c token.h
token.o: token.c token.h
parser.o: parser.c parser.h
error.o: error.c error.h

test: ncc
	./test.sh

clean:
	rm -f ncc *.o *~ tmp*

.PHONY: test clean
