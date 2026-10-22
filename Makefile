CC = cc
CFLAGS = -g -Wall -Wextra -Isrc

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

ncc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: ncc
	./test.sh

clean:
	rm -f ncc src/*.o tmp*

.PHONY: test clean
