# Explicit declarations
assert 5 'a: int=5; return a;'
assert 10 'a: int; a=10; return a;'
assert 15 'a: int=5; b: int=10; return a+b;'
assert 7 'a: int=5; b: int=a+2; return b;'
assert 10 'a: int=5; a=10; return a;'
assert 6 'a: int=5; return ++a;'

# Parameters are already declared locals
assert_program 6 'int increment(a: int) { b: int=a+1; return b; } int main() { return increment(5); }'

# Invalid variable uses
assert_error 'Undeclared variable' 'int main() { a=5; return a; }'
assert_error 'Undeclared variable' 'int main() { return missing; }'
assert_error 'Variable already declared' 'int main() { a: int; a: int; return 0; }'

