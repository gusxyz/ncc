# Multiple statements
assert 3 '1; 2; return 3;'
assert 15 '5; 10; return 15;'
assert 42 '1+2; return 6*7;'
assert 9 '2+3; return 4+5;'
assert 20 '10; return 5*4;'

# Local variables (explicit declarations required)
assert 5 'a: int; a=5; return a;'
assert 10 'a: int; a=10; return a;'
assert 15 'a: int; b: int; a=5; b=10; return a+b;'
assert 20 'a: int; b: int; a=10; b=2; return a*b;'
assert 5 'a: int; b: int; a=20; b=4; return a/b;'
assert 1 'a: int; b: int; a=5; b=5; return a==b;'
assert 1 'a: int; b: int; a=3; b=7; return a<b;'
assert 1 'a: int; b: int; a=7; b=3; return a>b;'

# Reassignment
assert 10 'a: int; a=5; a=10; return a;'
assert 15 'a: int; a=5; a=a+10; return a;'
assert 25 'a: int; a=5; a=a*a; return a;'
assert 3 'a: int; a=10; a=a-7; return a;'

# Chained assignments
assert 14 'a: int; b: int; a=b=7; return a+b;'
assert 30 'a: int; b: int; c: int; a=b=c=10; return a+b+c;'
assert 42 'a: int; b: int; a=b=21; return a+b;'
assert 5 'a: int; b: int; a=b=5; return a;'

# Assignment expressions
assert 10 'a: int; return (a=5)+a;'
assert 15 'a: int; b: int; a=5; b=(a=10); return a+b-5;'
assert 1 'a: int; return (a=5)==5;'

# Variable precedence
assert 14 'a: int; b: int; a=2; b=3; return a+b*4;'
assert 20 'a: int; b: int; a=2; b=3; return (a+b)*4;'
assert 1 'a: int; b: int; a=5; b=10; return a*2==b;'
assert 1 'a: int; b: int; a=5; b=10; return a<b;'

# Multi-character variable names
assert 10 'foo: int; bar: int; foo=5; bar=5; return foo+bar;'

