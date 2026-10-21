CLAFGS=-g -static

ncc: ncc.c

test: ncc
  ./test.sh

clean:
  rm -f ncc *.o *~ tmp*

.PHONY: test clean
