#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  ./ncc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}


# Addition
assert 0 '0;'
assert 42 '42;'
assert 10 '5+5;'
assert 100 '50+50;'
assert 21 '1+2+3+4+5+6;'

# Subtraction
assert 5 '10-5;'
assert 0 '5-5;'
assert 10 '20-5-5;'
assert 3 '10-4-3;'

# Multiplication
assert 42 '6*7;'
assert 100 '10*10;'
assert 24 '2*3*4;'
assert 0 '100*0;'

# Division
assert 5 '10/2;'
assert 3 '10/3;'
assert 2 '8/4;'
assert 2 '20/5/2;'

# Operator precedence
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 14 '2+3*4;'
assert 20 '(2+3)*4;'
assert 7 '1+2*3;'
assert 9 '1+2*3+2;'
assert 26 '2*3+4*5;'
assert 14 '20-3*2;'
assert 10 '100/5/2;'
assert 17 '2+3*5;'

# Nested parentheses
assert 21 '(1+2)*(3+4);'
assert 20 '((2+3)*4);'
assert 10 '(((10)));'
assert 14 '2*(3+(4));'
assert 45 '(2+3)*(4+5);'
assert 3 '(10-(4+3));'

# Unary operators
assert 5 '+5;'
assert 5 '--5;'
assert 5 '-(-5);'
assert 251 '+-+--5;' # -5 exit status = 251
assert 0 '-0;'
assert 10 '5--5;'
assert 2 '5+-3;'

# Equality
assert 1 '5==5;'
assert 0 '5==6;'
assert 1 '5!=6;'
assert 0 '5!=5;'
assert 1 '2+3==5;'
assert 1 '2*3==3*2;'
assert 0 '2+3!=5;'

# Less than
assert 1 '3<5;'
assert 0 '5<3;'
assert 0 '5<5;'
assert 1 '3<=5;'
assert 1 '5<=5;'
assert 0 '6<=5;'

# Greater than
assert 1 '5>3;'
assert 0 '3>5;'
assert 0 '5>5;'
assert 1 '5>=3;'
assert 1 '5>=5;'
assert 0 '3>=5;'

# Comparison precedence
assert 1 '2+3==5;'
assert 1 '2*3>5;'
assert 1 '10/2<=5;'
assert 1 '2+3*4==14;'
assert 1 '(2+3)*4>15;'
assert 0 '2+3*4<10;'
assert 1 '5+5>=2*5;'

# Chained comparisons (left-associative)
assert 1 '1<2<3;'
assert 1 '3<2<1;'
assert 1 '3>2>0;'
assert 1 '1==1==1;'
assert 0 '1==1==0;'
assert 1 '1<2==1;'
assert 1 '5>3!=0;'

# Negative values in comparisons
assert 1 '-5<3;'
assert 1 '-5<-3;'
assert 1 '-3>-5;'
assert 1 '-5<=-5;'
assert 1 '-3>=-5;'
assert 0 '-5>3;'

# Integer division truncation
assert 3 '10/3;'
assert 2 '8/3;'
assert 0 '1/2;'
assert 0 '0/5;'
assert 5 '11/2;'
assert 1 '5/3;'

# Multiple statements
assert 3 '1; 2; 3;'
assert 15 '5; 10; 15;'
assert 42 '1+2; 6*7;'
assert 9 '2+3; 4+5;'
assert 20 '10; 5*4;'

# Local variables
assert 5 'a=5;'
assert 10 'a=10; a;'
assert 15 'a=5; b=10; a+b;'
assert 20 'a=10; b=2; a*b;'
assert 5 'a=20; b=4; a/b;'
assert 1 'a=5; b=5; a==b;'
assert 1 'a=3; b=7; a<b;'
assert 1 'a=7; b=3; a>b;'

# Reassignment
assert 10 'a=5; a=10; a;'
assert 15 'a=5; a=a+10; a;'
assert 25 'a=5; a=a*a; a;'
assert 3 'a=10; a=a-7; a;'

# Chained assignments
assert 14 'a=b=7; a+b;'
assert 30 'a=b=c=10; a+b+c;'
assert 42 'a=b=21; a+b;'
assert 5 'a=b=5; a;'

# Assignment expressions
assert 10 '(a=5)+a;'
assert 15 'a=5; b=(a=10); a+b-5;'
assert 1 '(a=5)==5;'

# Variable precedence
assert 14 'a=2; b=3; a+b*4;'
assert 20 'a=2; b=3; (a+b)*4;'
assert 1 'a=5; b=10; a*2==b;'
assert 1 'a=5; b=10; a<b;'

# Return & Multi-Character Variable expressions
assert 10 'foo=5; bar=5; return foo+bar;'
echo OK
