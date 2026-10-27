# Prefix increment
assert 6 'i: int; i=5; return ++i;'
assert 6 'i: int; i=5; ++i; return i;'
assert 12 'i: int; i=5; return ++i*2;'
assert 7 'i: int; a: int; i=5; a=++i; return a+1;'
assert 3 'i: int; i=0; ++i; ++i; ++i; return i;'

# Postfix increment
assert 5 'i: int; i=5; return i++;'
assert 6 'i: int; i=5; i++; return i;'
assert 10 'i: int; i=5; return i++*2;'
assert 11 'i: int; a: int; i=5; a=i++; return a+i;'
assert 16 'i: int; a: int; i=5; a=i++*2; return a+i;'
assert 3 'i: int; i=0; i++; i++; i++; return i;'

# Prefix decrement
assert 4 'i: int; i=5; return --i;'
assert 4 'i: int; i=5; --i; return i;'
assert 8 'i: int; i=5; return --i*2;'
assert 8 'i: int; a: int; i=5; a=--i; return a+i;'
assert 2 'i: int; i=5; --i; --i; --i; return i;'

# Postfix decrement
assert 5 'i: int; i=5; return i--;'
assert 4 'i: int; i=5; i--; return i;'
assert 10 'i: int; i=5; return i--*2;'
assert 9 'i: int; a: int; i=5; a=i--; return a+i;'
assert 14 'i: int; a: int; i=5; a=i--*2; return a+i;'
assert 2 'i: int; i=5; i--; i--; i--; return i;'

# Mixed increment/decrement
assert 5 'i: int; i=5; i++; i--; return i;'
assert 5 'i: int; i=5; ++i; --i; return i;'
assert 6 'i: int; i=5; ++i; i--; i++; return i;'
assert 10 'i: int; a: int; b: int; i=5; a=i++; b=++i; return a+b-2;'
assert 0 'i: int; i=0; i++; i--; return i;'

