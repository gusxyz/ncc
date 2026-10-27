# If statements
assert 10 'if (1) return 10; return 20;'
assert 20 'foo: int; bar: int; foo=5; bar=2; if (foo<bar) return 10; return 20;'
assert 10 'if (1) return 10; else return 20;'
assert 20 'if (0) return 10; else return 20;'
assert 30 'if (1) if (0) return 10; else return 30; return 20;'
assert 40 'if (0) return 10; else if (1) return 40; else return 20;'

# While loops
assert 5 'a: int; a=0; while(a<5) a=a+1; return a;'
assert 10 'a: int; a=0; while(a<10) a=a+1; return a;'
assert 0 'a: int; a=0; while(0) a=a+1; return a;'
assert 10 'a: int; a=10; while(a<5) a=a+1; return a;'
assert 0 'a: int; a=10; while(a>0) a=a-1; return a;'
assert 6 'a: int; b: int; a=0; while(a<3) a=a+1; b=0; while(b<3) b=b+1; return a+b;'
assert 10 'a: int; a=0; while(a<10) if(a<5) a=a+2; else a=a+1; return a;'

# Blocks
assert 10 '{ return 10; }'
assert 20 'a: int; b: int; { a=10; b=10; return a+b; }'
assert 42 'a: int; a=1; { a=42; } return a;'
assert 55 'a: int; sum: int; a=1; sum=0; while(a<=10) { sum=sum+a; a=a+1; } return sum;'
assert 30 'a: int; a=0; if(1) { a=10; a=a+20; } else { a=100; } return a;'
assert 100 'a: int; a=0; if(0) { a=10; } else { a=100; } return a;'
assert 15 'a: int; b: int; { a=5; { b=10; } return a+b; }'

