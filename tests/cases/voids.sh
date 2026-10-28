assert_program 42 'int result; void set() { result=42; return; } int main() { set(); return result; }'
assert_program 42 'void nothing() {} int main() { nothing(); return 42; }'