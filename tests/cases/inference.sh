# type inference tests
assert 40 'x := 42; ptr := &x; y := -2; return *ptr+y;'
assert 1 'x := 6*7; same := x==42; return same;'

assert 67 'X :: 67; return X;'
