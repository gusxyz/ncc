# Function definitions and calls (no parameters yet)
assert_program 42 'int answer() { return 42; } int main() { return answer(); }'
assert_program 42 'int main() { return answer(); } int answer() { return 42; }'
assert_program 42 'int six() { return 6; } int seven() { return 7; } int main() { return six()*seven(); }'
assert_program 47 'int answer() { return 42; } int main() { return 5+answer(); }'
assert_program 47 'int answer() { return 42; } int main() { return answer()+5; }'
assert_program 84 'int answer() { return 42; } int main() { return answer()+answer(); }'
assert_program 42 'int leaf() { return 42; } int middle() { return leaf(); } int main() { return middle(); }'

# Caller variables survive calls; each function has its own frame.
assert_program 12 'int helper() { a: int; a=7; return a; } int main() { a: int; b: int; a=5; b=helper(); return a+b; }'
assert_program 5 'int helper() { a: int; a=99; return a; } int main() { a: int; a=5; helper(); return a; }'
assert_program 10 'int one() { return 1; } int main() { sum: int; i: int; sum=0; for(i=0;i<10;i++) sum=sum+one(); return sum; }'

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
assert_program 15 'int add(int a, int b) { return a+b; } int main() { x: int; y: int; x=5; y=10; return add(x,y); }'

# All six registers; subtraction makes argument order matter
assert_program 43 'int six(int a, int b, int c, int d, int e, int f) { return a-b-c-d-e-f; } int main() { return six(63,1,2,4,5,8); }'

# Nested calls must preserve already evaluated arguments
assert_program 42 'int identity(int a) { return a; } int add(int a, int b) { return a+b; } int main() { return add(identity(20),identity(22)); }'

# Preserve a pending expression value across a call
assert_program 47 'int add(int a, int b) { return a+b; } int main() { return 5+add(20,22); }'

# Changing a parameter must not change the caller's variable
assert_program 5 'int change(int a) { a=99; return a; } int main() { a: int=5; change(a); return a; }'

# Recursion: every call needs its own parameter storage
assert_program 120 'int factorial(int n) { if(n<=1) return 1; return n*factorial(n-1); } int main() { return factorial(5); }'

