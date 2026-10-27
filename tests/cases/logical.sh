# nots
assert 1 'return !0;'
assert 0 'return !42;'
assert 0 'return !-1;'
assert 1 'return !!42;'
assert 0 'return !!0;'
assert 1 'x := 0; return !x;'
assert 1 'return !(2>3);'
assert 1 'x := 0; return !(x=0);'

# ands
assert 0 'return 0 && 42;'
assert 1 'return 2 && 42;'
assert 0 'x := 0; 0 && (x=1); return x;'
assert 1 'x := 0; 1 && (x=1); return x;'

# ors
assert 0 'return 0 || 0;'
assert 1 'return 0 || 42;'
assert 1 'return 1 || 0 && 0;'
assert 0 'x := 0; 1 || (x=1); return x;'
assert 1 'x := 0; 0 || (x=1); return x;'
