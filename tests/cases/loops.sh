# Basic for loops
assert 5 'i: int; for(i=0;i<5;i++) {} return i;'
assert 10 'i: int; for(i=0;i<10;++i) {} return i;'
assert 10 'a: int; i: int; a=0; for(i=0;i<10;i++) a++; return a;'
assert 45 'sum: int; i: int; sum=0; for(i=0;i<10;i++) {sum=sum+i;} return sum;'
assert 55 'sum: int; i: int; sum=0; for(i=1;i<=10;i++) {sum=sum+i;} return sum;'

# For loops with decrement
assert 0 'i: int; for(i=10;i>0;i--) {} return i;'
assert 0 'i: int; for(i=10;i>0;--i) {} return i;'
assert 55 'sum: int; i: int; sum=0; for(i=10;i>0;i--) {sum=sum+i;} return sum;'
assert 5 'i: int; for(i=10;i>5;i--) {} return i;'

# Optional for expressions
assert 5 'i: int; i=0; for(;i<5;i++) {} return i;'
assert 5 'i: int; i=0; for(;i<5;) i++; return i;'
assert 5 'i: int; i=0; for(;;) {if(i==5) return i; i++;}'
assert 42 'for(;;) return 42;'
assert 0 'i: int; for(i=0;i<0;i++) {} return i;'

# Nested for loops
assert 9 'sum: int; i: int; j: int; sum=0; for(i=0;i<3;i++) {for(j=0;j<3;j++) sum++;} return sum;'
assert 25 'sum: int; i: int; j: int; sum=0; for(i=0;i<5;i++) {for(j=0;j<5;j++) sum++;} return sum;'
assert 100 'sum: int; i: int; j: int; sum=0; for(i=0;i<10;i++) {for(j=0;j<10;j++) sum++;} return sum;'

# For loops with conditionals
assert 5 'i: int; for(i=0;i<10;i++) {if(i==5) return i;} return 0;'
assert 7 'i: int; for(i=0;i<10;i++) {if(i==7) return i;} return 0;'
assert 10 'a: int; i: int; a=0; for(i=0;i<10;i++) {if(i<5) a++; else a++;} return a;'

# Loop/stack balance
assert 100 'i: int; j: int; i=0; for(j=0;j<100;j++) {i++;} return i;'
assert 200 'i: int; j: int; i=0; for(j=0;j<200;j++) {++i;} return i;'
assert 50 'i: int; j: int; i=100; for(j=0;j<50;j++) {i--;} return i;'
assert 42 'i: int; j: int; i=0; for(j=0;j<42;j++) {i=i+1;} return i;'

# Break and continue
assert 5 'i: int; for(i=0;i<10;i++) {if(i==5) break;} return i;'
assert 3 'i: int; i=0; while(i<10) {if(i==3) break; i++;} return i;'
assert 5 'i: int; sum: int; i=0; sum=0; while(i<5) {i++; if(i<5) continue; sum=sum+i;} return sum;'
assert 5 'sum: int; i: int; sum=0; for(i=0;i<10;i++) {if(i<5) continue; sum++;} return sum;'

