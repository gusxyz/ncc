#!/bin/bash

set -o pipefail

assert() {
  local expected="$1"
  local input="$2"
  local actual

  ./build/ncc "$input" > build/tmp.s || exit 1
  cc -o build/tmp build/tmp.s || exit 1
  ./build/tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# Addition
assert 0 'return 0;'
assert 42 'return 42;'
assert 10 'return 5+5;'
assert 100 'return 50+50;'
assert 21 'return 1+2+3+4+5+6;'

# Subtraction
assert 5 'return 10-5;'
assert 0 'return 5-5;'
assert 10 'return 20-5-5;'
assert 3 'return 10-4-3;'

# Multiplication
assert 42 'return 6*7;'
assert 100 'return 10*10;'
assert 24 'return 2*3*4;'
assert 0 'return 100*0;'

# Division
assert 5 'return 10/2;'
assert 3 'return 10/3;'
assert 2 'return 8/4;'
assert 2 'return 20/5/2;'

# Operator precedence
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 14 'return 2+3*4;'
assert 20 'return (2+3)*4;'
assert 7 'return 1+2*3;'
assert 9 'return 1+2*3+2;'
assert 26 'return 2*3+4*5;'
assert 14 'return 20-3*2;'
assert 10 'return 100/5/2;'
assert 17 'return 2+3*5;'

# Nested parentheses
assert 21 'return (1+2)*(3+4);'
assert 20 'return ((2+3)*4);'
assert 10 'return (((10)));'
assert 14 'return 2*(3+(4));'
assert 45 'return (2+3)*(4+5);'
assert 3 'return (10-(4+3));'

# Unary operators
assert 5 'return +5;'
assert 5 'return --5;'
assert 5 'return -(-5);'
assert 251 'return +-+--5;' # -5 exit status = 251
assert 0 'return -0;'
assert 10 'return 5--5;'
assert 2 'return 5+-3;'

# Equality
assert 1 'return 5==5;'
assert 0 'return 5==6;'
assert 1 'return 5!=6;'
assert 0 'return 5!=5;'
assert 1 'return 2+3==5;'
assert 1 'return 2*3==3*2;'
assert 0 'return 2+3!=5;'

# Less than
assert 1 'return 3<5;'
assert 0 'return 5<3;'
assert 0 'return 5<5;'
assert 1 'return 3<=5;'
assert 1 'return 5<=5;'
assert 0 'return 6<=5;'

# Greater than
assert 1 'return 5>3;'
assert 0 'return 3>5;'
assert 0 'return 5>5;'
assert 1 'return 5>=3;'
assert 1 'return 5>=5;'
assert 0 'return 3>=5;'

# Comparison precedence
assert 1 'return 2+3==5;'
assert 1 'return 2*3>5;'
assert 1 'return 10/2<=5;'
assert 1 'return 2+3*4==14;'
assert 1 'return (2+3)*4>15;'
assert 0 'return 2+3*4<10;'
assert 1 'return 5+5>=2*5;'

# Chained comparisons (left-associative)
assert 1 'return 1<2<3;'
assert 1 'return 3<2<1;'
assert 1 'return 3>2>0;'
assert 1 'return 1==1==1;'
assert 0 'return 1==1==0;'
assert 1 'return 1<2==1;'
assert 1 'return 5>3!=0;'

# Negative values in comparisons
assert 1 'return -5<3;'
assert 1 'return -5<-3;'
assert 1 'return -3>-5;'
assert 1 'return -5<=-5;'
assert 1 'return -3>=-5;'
assert 0 'return -5>3;'

# Integer division truncation
assert 3 'return 10/3;'
assert 2 'return 8/3;'
assert 0 'return 1/2;'
assert 0 'return 0/5;'
assert 5 'return 11/2;'
assert 1 'return 5/3;'

# Multiple statements
assert 3 '1; 2; return 3;'
assert 15 '5; 10; return 15;'
assert 42 '1+2; return 6*7;'
assert 9 '2+3; return 4+5;'
assert 20 '10; return 5*4;'

# Local variables
assert 5 'a=5; return a;'
assert 10 'a=10; return a;'
assert 15 'a=5; b=10; return a+b;'
assert 20 'a=10; b=2; return a*b;'
assert 5 'a=20; b=4; return a/b;'
assert 1 'a=5; b=5; return a==b;'
assert 1 'a=3; b=7; return a<b;'
assert 1 'a=7; b=3; return a>b;'

# Reassignment
assert 10 'a=5; a=10; return a;'
assert 15 'a=5; a=a+10; return a;'
assert 25 'a=5; a=a*a; return a;'
assert 3 'a=10; a=a-7; return a;'

# Chained assignments
assert 14 'a=b=7; return a+b;'
assert 30 'a=b=c=10; return a+b+c;'
assert 42 'a=b=21; return a+b;'
assert 5 'a=b=5; return a;'

# Assignment expressions
assert 10 'return (a=5)+a;'
assert 15 'a=5; b=(a=10); return a+b-5;'
assert 1 'return (a=5)==5;'

# Variable precedence
assert 14 'a=2; b=3; return a+b*4;'
assert 20 'a=2; b=3; return (a+b)*4;'
assert 1 'a=5; b=10; return a*2==b;'
assert 1 'a=5; b=10; return a<b;'

# Return & Multi-Character Variable expressions
assert 10 'foo=5; bar=5; return foo+bar;'

# if statements
assert 10 'if (1) return 10; return 20;'
assert 20 'foo = 5; bar = 2; if (foo < bar) return 10; return 20;'

# if / else statements
assert 10 'if (1) return 10; else return 20;'
assert 20 'if (0) return 10; else return 20;'

# Nested if / else
assert 30 'if (1) if (0) return 10; else return 30; return 20;'
assert 40 'if (0) return 10; else if (1) return 40; else return 20;'

# While loops
assert 5 'a=0; while(a<5) a=a+1; return a;'
assert 10 'a=0; while(a<10) a=a+1; return a;'
assert 0 'a=0; while(0) a=a+1; return a;'
assert 10 'a=10; while(a<5) a=a+1; return a;'
assert 0 'a=10; while(a>0) a=a-1; return a;'

# Multiple loops
assert 6 'a=0; while(a<3) a=a+1; b=0; while(b<3) b=b+1; return a+b;'

# While + if/else
assert 10 'a=0; while(a<10) if(a<5) a=a+2; else a=a+1; return a;'

echo OK
