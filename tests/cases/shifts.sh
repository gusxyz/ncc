# Basic shifts
assert 8 'return 1<<3;'
assert 4 'return 16>>2;'
assert 42 'return 21<<1;'
assert 21 'return 42>>1;'

# Shift by zero
assert 42 'return 42<<0;'
assert 42 'return 42>>0;'

# Operands can be expressions
assert 24 'value := 3; amount := 3; return value<<amount;'
assert 8 'return 1<<(1+2);'

# Addition binds more tightly than shifts
assert 8 'return 1<<1+2;'
assert 4 'return 32>>1+2;'

# Shifts associate left to right
assert 16 'return 1<<2<<2;'
assert 4 'return 64>>2>>2;'

# Shifts bind more tightly than comparisons and bitwise AND
assert 1 'return 1<<3 == 8;'
assert 1 'return 16>>2 < 5;'
assert 8 'return 1<<3 & 8;'

# Signed right shift
assert 1 'return (-8>>2)==-2;'
assert 1 'return (-1>>10)==-1;'

# Large shifts: compare internally to avoid exit-status truncation
assert 1 'return (1<<40)>>40 == 1;'
assert 1 'return (1<<62)>>62 == 1;'

# Both operands are evaluated once
assert 1 'x := 2; y := 1; result := x++<<y++; return result==4 && x==3 && y==2;'