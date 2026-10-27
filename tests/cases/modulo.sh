assert 1 'return 10%3;'
assert 0 'return 12%4;'
assert 2 'return 2%5;'
assert 0 'return 0%7;'
assert 1 'return 17%4%2;'

assert 7 'return 5+8%3;'
assert 4 'return 10%3*4;'
assert 2 'return 20/3%4;'

assert 1 'return -10%3 == -1;'
assert 1 'return 10%-3 == 1;'
assert 1 'return -10%-3 == -1;'

assert 2 'x := 17; y := 5; return x%y;'
assert 1 'x := 0; for(i := 0; i<10; i++) if(i%2==0) x++; return x==5;'