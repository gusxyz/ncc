# Pointers: address-of and dereference
assert 42 'x: int=42; p: int*; p=&x; return *p;'
assert 9 'x: int; *(&x)=9; return x;'
assert 7 'x: int=1; p: int*; p=&x; *p=7; return x;'

# Pointer aliases and reassignment
assert 42 'x: int=42; p: int*; q: int*; p=&x; q=p; return *q;'
assert 7 'x: int=1; y: int=7; p: int*; p=&x; p=&y; return *p;'

# Pointers to u8
assert 255 'x: u8=255; p: u8*; p=&x; return *p;'
assert 9 'x: u8=1; p: u8*; p=&x; *p=9; return x;'

