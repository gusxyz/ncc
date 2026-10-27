# Global Variable
assert_program 5 'int x; int main() { x = 5; return x; }'
assert_program 37 'int counter; int set() { counter = 37; return 0; } int main() { set(); return counter; }'

