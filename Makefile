CC = cc
CFLAGS = -g -Wall -Wextra

OBJS = ncc.o token.o

ncc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

ncc.o: ncc.c token.h
token.o: token.c token.h

test: ncc
	./test.sh

clean:
	rm -f ncc *.o *~ tmp*

.PHONY: test clean
