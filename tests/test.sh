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
assert 5 'return -(-5);'
assert 5 'return - -5;'
assert 251 'return +-(+(-(-5)));'

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
assert 10 '{ return 10; }'
assert 20 '{ a=10; b=10; return a+b; }'
assert 42 'a=1; { a=42; } return a;'

# While loops with blocks
assert 55 'a=1; sum=0; while(a<=10) { sum=sum+a; a=a+1; } return sum;'

# If/else with blocks
assert 30 'a=0; if(1) { a=10; a=a+20; } else { a=100; } return a;'
assert 100 'a=0; if(0) { a=10; } else { a=100; } return a;'

# Nested blocks
assert 15 '{ a=5; { b=10; } return a+b; }'

# ==========================================
# PREFIX INCREMENT (++i)
# ==========================================
assert 6 'i=5; return ++i;'
assert 6 'i=5; ++i; return i;'
assert 12 'i=5; return ++i*2;'
assert 7 'i=5; a=++i; return a+1;'
assert 3 'i=0; ++i; ++i; ++i; return i;'

# ==========================================
# POSTFIX INCREMENT (i++)
# ==========================================
assert 5 'i=5; return i++;'
assert 6 'i=5; i++; return i;'
assert 10 'i=5; return i++*2;'
assert 11 'i=5; a=i++; return a+i;'
assert 16 'i=5; a=i++*2; return a+i;'
assert 3 'i=0; i++; i++; i++; return i;'

# ==========================================
# PREFIX DECREMENT (--i)
# ==========================================
assert 4 'i=5; return --i;'
assert 4 'i=5; --i; return i;'
assert 8 'i=5; return --i*2;'
assert 8 'i=5; a=--i; return a+i;'
assert 2 'i=5; --i; --i; --i; return i;'

# ==========================================
# POSTFIX DECREMENT (i--)
# ==========================================
assert 5 'i=5; return i--;'
assert 4 'i=5; i--; return i;'
assert 10 'i=5; return i--*2;'
assert 9 'i=5; a=i--; return a+i;'
assert 14 'i=5; a=i--*2; return a+i;'
assert 2 'i=5; i--; i--; i--; return i;'

# ==========================================
# MIXED INCREMENT / DECREMENT
# ==========================================
assert 5 'i=5; i++; i--; return i;'
assert 5 'i=5; ++i; --i; return i;'
assert 6 'i=5; ++i; i--; i++; return i;'
assert 10 'i=5; a=i++; b=++i; return a+b-2;'
assert 0 'i=0; i++; i--; return i;'

# ==========================================
# BASIC FOR LOOPS
# ==========================================
assert 5 'for(i=0;i<5;i++) {} return i;'
assert 10 'for(i=0;i<10;++i) {} return i;'
assert 10 'a=0; for(i=0;i<10;i++) a++; return a;'
assert 45 'sum=0; for(i=0;i<10;i++) {sum=sum+i;} return sum;'
assert 55 'sum=0; for(i=1;i<=10;i++) {sum=sum+i;} return sum;'

# ==========================================
# FOR LOOPS WITH DECREMENT
# ==========================================
assert 0 'for(i=10;i>0;i--) {} return i;'
assert 0 'for(i=10;i>0;--i) {} return i;'
assert 55 'sum=0; for(i=10;i>0;i--) {sum=sum+i;} return sum;'
assert 5 'for(i=10;i>5;i--) {} return i;'

# ==========================================
# OPTIONAL FOR EXPRESSIONS
# ==========================================
assert 5 'i=0; for(;i<5;i++) {} return i;'
assert 5 'i=0; for(;i<5;) i++; return i;'
assert 5 'i=0; for(;;) {if(i==5) return i; i++;}'
assert 42 'for(;;) return 42;'
assert 0 'for(i=0;i<0;i++) {} return i;'

# ==========================================
# NESTED FOR LOOPS
# ==========================================
assert 9 'sum=0; for(i=0;i<3;i++) {for(j=0;j<3;j++) sum++;} return sum;'
assert 25 'sum=0; for(i=0;i<5;i++) {for(j=0;j<5;j++) sum++;} return sum;'
assert 100 'sum=0; for(i=0;i<10;i++) {for(j=0;j<10;j++) sum++;} return sum;'

# ==========================================
# FOR LOOPS WITH CONDITIONALS
# ==========================================
assert 5 'for(i=0;i<10;i++) {if(i==5) return i;} return 0;'
assert 7 'for(i=0;i<10;i++) {if(i==7) return i;} return 0;'
assert 10 'a=0; for(i=0;i<10;i++) {if(i<5) a++; else a++;} return a;'

# ==========================================
# LOOP / STACK BALANCE
# ==========================================
assert 100 'i=0; for(j=0;j<100;j++) {i++;} return i;'
assert 200 'i=0; for(j=0;j<200;j++) {++i;} return i;'
assert 50 'i=100; for(j=0;j<50;j++) {i--;} return i;'
assert 42 'i=0; for(j=0;j<42;j++) {i=i+1;} return i;'

# Break
assert 5 'for(i=0;i<10;i++) {if(i==5) break;} return i;'
assert 3 'i=0; while(i<10) {if(i==3) break; i++;} return i;'

# Continue in while
assert 5 'i=0; sum=0; while(i<5) {i++; if(i<5) continue; sum=sum+i;} return sum;'

# Continue in for
assert 5 'sum=0; for(i=0;i<10;i++) {if(i<5) continue; sum++;} return sum;'

echo OK
