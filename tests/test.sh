#!/bin/bash

# Run from the project root: bash tests/test.sh
# Deliberately omit set -e: nonzero program exits are test results.
set -uo pipefail

passed=0
mkdir -p build || exit 1

# Test a complete program, including its function definitions.
assert_program() {
  local expected="$1"
  local input="$2"
  local actual

  if ! ./build/ncc "$input" > build/tmp.s; then
    printf 'FAIL: compilation failed\nSource: %s\n' "$input" >&2
    exit 1
  fi
  if ! cc -o build/tmp build/tmp.s; then
    printf 'FAIL: assembly/linking failed\nSource: %s\n' "$input" >&2
    exit 1
  fi

  ./build/tmp
  actual=$?

  if [ "$actual" -ne "$expected" ]; then
    printf 'FAIL: expected %s, got %s\nSource: %s\n' "$expected" "$actual" "$input" >&2
    exit 1
  fi

  printf '%s => %s\n' "$input" "$actual"
  passed=$((passed + 1))
}

# Existing tests supply only a body; give each an explicit main.
assert() {
  assert_program "$1" "int main() { $2 }"
}

# Require a compiler error containing the expected diagnostic.
assert_error() {
  local expected="$1"
  local input="$2"

  if ./build/ncc "$input" > build/tmp.s 2> build/tmp.err; then
    printf 'FAIL: expected compiler rejection\nSource: %s\n' "$input" >&2
    exit 1
  fi
  if ! grep -Fq -- "$expected" build/tmp.err; then
    printf 'FAIL: expected diagnostic containing: %s\n' "$expected" >&2
    cat build/tmp.err >&2
    exit 1
  fi

  printf 'Rejected as expected: %s\n' "$input"
  passed=$((passed + 1))
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

# Unary operators; process exit statuses wrap to 0..255.
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

# Local variables (implicit declarations remain supported)
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

# Multi-character variable names
assert 10 'foo=5; bar=5; return foo+bar;'

# If statements
assert 10 'if (1) return 10; return 20;'
assert 20 'foo=5; bar=2; if (foo<bar) return 10; return 20;'
assert 10 'if (1) return 10; else return 20;'
assert 20 'if (0) return 10; else return 20;'
assert 30 'if (1) if (0) return 10; else return 30; return 20;'
assert 40 'if (0) return 10; else if (1) return 40; else return 20;'

# While loops
assert 5 'a=0; while(a<5) a=a+1; return a;'
assert 10 'a=0; while(a<10) a=a+1; return a;'
assert 0 'a=0; while(0) a=a+1; return a;'
assert 10 'a=10; while(a<5) a=a+1; return a;'
assert 0 'a=10; while(a>0) a=a-1; return a;'
assert 6 'a=0; while(a<3) a=a+1; b=0; while(b<3) b=b+1; return a+b;'
assert 10 'a=0; while(a<10) if(a<5) a=a+2; else a=a+1; return a;'

# Blocks
assert 10 '{ return 10; }'
assert 20 '{ a=10; b=10; return a+b; }'
assert 42 'a=1; { a=42; } return a;'
assert 55 'a=1; sum=0; while(a<=10) { sum=sum+a; a=a+1; } return sum;'
assert 30 'a=0; if(1) { a=10; a=a+20; } else { a=100; } return a;'
assert 100 'a=0; if(0) { a=10; } else { a=100; } return a;'
assert 15 '{ a=5; { b=10; } return a+b; }'

# Prefix increment
assert 6 'i=5; return ++i;'
assert 6 'i=5; ++i; return i;'
assert 12 'i=5; return ++i*2;'
assert 7 'i=5; a=++i; return a+1;'
assert 3 'i=0; ++i; ++i; ++i; return i;'

# Postfix increment
assert 5 'i=5; return i++;'
assert 6 'i=5; i++; return i;'
assert 10 'i=5; return i++*2;'
assert 11 'i=5; a=i++; return a+i;'
assert 16 'i=5; a=i++*2; return a+i;'
assert 3 'i=0; i++; i++; i++; return i;'

# Prefix decrement
assert 4 'i=5; return --i;'
assert 4 'i=5; --i; return i;'
assert 8 'i=5; return --i*2;'
assert 8 'i=5; a=--i; return a+i;'
assert 2 'i=5; --i; --i; --i; return i;'

# Postfix decrement
assert 5 'i=5; return i--;'
assert 4 'i=5; i--; return i;'
assert 10 'i=5; return i--*2;'
assert 9 'i=5; a=i--; return a+i;'
assert 14 'i=5; a=i--*2; return a+i;'
assert 2 'i=5; i--; i--; i--; return i;'

# Mixed increment/decrement
assert 5 'i=5; i++; i--; return i;'
assert 5 'i=5; ++i; --i; return i;'
assert 6 'i=5; ++i; i--; i++; return i;'
assert 10 'i=5; a=i++; b=++i; return a+b-2;'
assert 0 'i=0; i++; i--; return i;'

# Basic for loops
assert 5 'for(i=0;i<5;i++) {} return i;'
assert 10 'for(i=0;i<10;++i) {} return i;'
assert 10 'a=0; for(i=0;i<10;i++) a++; return a;'
assert 45 'sum=0; for(i=0;i<10;i++) {sum=sum+i;} return sum;'
assert 55 'sum=0; for(i=1;i<=10;i++) {sum=sum+i;} return sum;'

# For loops with decrement
assert 0 'for(i=10;i>0;i--) {} return i;'
assert 0 'for(i=10;i>0;--i) {} return i;'
assert 55 'sum=0; for(i=10;i>0;i--) {sum=sum+i;} return sum;'
assert 5 'for(i=10;i>5;i--) {} return i;'

# Optional for expressions
assert 5 'i=0; for(;i<5;i++) {} return i;'
assert 5 'i=0; for(;i<5;) i++; return i;'
assert 5 'i=0; for(;;) {if(i==5) return i; i++;}'
assert 42 'for(;;) return 42;'
assert 0 'for(i=0;i<0;i++) {} return i;'

# Nested for loops
assert 9 'sum=0; for(i=0;i<3;i++) {for(j=0;j<3;j++) sum++;} return sum;'
assert 25 'sum=0; for(i=0;i<5;i++) {for(j=0;j<5;j++) sum++;} return sum;'
assert 100 'sum=0; for(i=0;i<10;i++) {for(j=0;j<10;j++) sum++;} return sum;'

# For loops with conditionals
assert 5 'for(i=0;i<10;i++) {if(i==5) return i;} return 0;'
assert 7 'for(i=0;i<10;i++) {if(i==7) return i;} return 0;'
assert 10 'a=0; for(i=0;i<10;i++) {if(i<5) a++; else a++;} return a;'

# Loop/stack balance
assert 100 'i=0; for(j=0;j<100;j++) {i++;} return i;'
assert 200 'i=0; for(j=0;j<200;j++) {++i;} return i;'
assert 50 'i=100; for(j=0;j<50;j++) {i--;} return i;'
assert 42 'i=0; for(j=0;j<42;j++) {i=i+1;} return i;'

# Break and continue
assert 5 'for(i=0;i<10;i++) {if(i==5) break;} return i;'
assert 3 'i=0; while(i<10) {if(i==3) break; i++;} return i;'
assert 5 'i=0; sum=0; while(i<5) {i++; if(i<5) continue; sum=sum+i;} return sum;'
assert 5 'sum=0; for(i=0;i<10;i++) {if(i<5) continue; sum++;} return sum;'

# Function definitions and calls (no parameters yet)
assert_program 42 'int answer() { return 42; } int main() { return answer(); }'
assert_program 42 'int main() { return answer(); } int answer() { return 42; }'
assert_program 42 'int six() { return 6; } int seven() { return 7; } int main() { return six()*seven(); }'
assert_program 47 'int answer() { return 42; } int main() { return 5+answer(); }'
assert_program 47 'int answer() { return 42; } int main() { return answer()+5; }'
assert_program 84 'int answer() { return 42; } int main() { return answer()+answer(); }'
assert_program 42 'int leaf() { return 42; } int middle() { return leaf(); } int main() { return middle(); }'

# Caller variables survive calls; each function has its own frame.
assert_program 12 'int helper() { a=7; return a; } int main() { a=5; b=helper(); return a+b; }'
assert_program 5 'int helper() { a=99; return a; } int main() { a=5; helper(); return a; }'
assert_program 10 'int one() { return 1; } int main() { sum=0; for(i=0;i<10;i++) sum=sum+one(); return sum; }'

# Control-flow labels must remain unique across functions.
assert_program 3 'int first() { if(1) return 1; return 0; } int second() { if(1) return 2; return 0; } int main() { return first()+second(); }'

# Falling off the end returns zero in the current NCC implementation.
assert_program 0 'int main() {}'
assert_program 0 'int helper() {} int main() { return helper(); }'

# A program must explicitly define main.
assert_error 'No defined main function' 'int helper() { return 42; }'
assert_error 'No defined main function' ''

# One argument
assert_program 42 'int identity(int a) { return a; } int main() { return identity(42); }'

# Argument order
assert_program 7 'int subtract(int a, int b) { return a-b; } int main() { return subtract(10,3); }'

# Expressions as arguments
assert_program 21 'int multiply(int a, int b) { return a*b; } int main() { return multiply(1+2,10-3); }'

# Local variables as arguments
assert_program 15 'int add(int a, int b) { return a+b; } int main() { x=5; y=10; return add(x,y); }'

# All six registers; subtraction makes argument order matter
assert_program 43 'int six(int a, int b, int c, int d, int e, int f) { return a-b-c-d-e-f; } int main() { return six(63,1,2,4,5,8); }'

# Nested calls must preserve already evaluated arguments
assert_program 42 'int identity(int a) { return a; } int add(int a, int b) { return a+b; } int main() { return add(identity(20),identity(22)); }'

# Preserve a pending expression value across a call
assert_program 47 'int add(int a, int b) { return a+b; } int main() { return 5+add(20,22); }'

# Changing a parameter must not change the caller's variable
assert_program 5 'int change(int a) { a=99; return a; } int main() { a=5; change(a); return a; }'

# Recursion: every call needs its own parameter storage
assert_program 120 'int factorial(int n) { if(n<=1) return 1; return n*factorial(n-1); } int main() { return factorial(5); }'

printf '\nAll %s tests passed.\n' "$passed"
