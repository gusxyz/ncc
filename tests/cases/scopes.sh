# Block scopes: an inner block can read and update an outer variable
assert 3 'x: int=1; { y: int=2; x=x+y; } return x;'

# A nested declaration shadows the outer variable, then the outer value remains
assert 1 'x: int=1; { x: int=2; if(x!=2) return 0; } return x;'

# Inner block can read a variable from its parent scope
assert 1 'x: int=1; { y: int=x+1; if(y!=2) return 0; } return x;'

# A variable declared in a block is unavailable after leaving it
assert_error 'Undeclared variable' 'int main() { { x: int=2; } return x; }'

# Same-scope duplicates are still rejected
assert_error 'Variable already declared' 'int main() { x: int=1; x: int=2; return x; }'

# A function body shares the parameter scope, so a local cannot redeclare a parameter
assert_error 'Variable already declared' 'int f(int x) { x: int=2; return x; } int main() { return f(1); }'

