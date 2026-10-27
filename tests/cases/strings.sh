# Char literals
assert 65 "return 'A';"
assert 65 "c: char='A'; return c;"

# String literals
assert 65 'return *"A";'
assert 104 's: char*="hello"; return *s;'
assert 0 's: char*=""; return *s;'

# Separate literals need separate labels
assert 195 'a: char*="a"; b: char*="b"; return *a+*b;'

# Passing a string to a function
assert_program 90 'int first(char *s) { return *s; } int main() { return first("Z"); }'

# Character escapes
assert 10 "return '\n';"
assert 9 "return '\t';"
assert 13 "return '\r';"
assert 0 "return '\0';"

# String escapes
assert 10 'return *"\n";'
assert 9 'return *"\t";'
assert 92 'return *"\\";'
assert 34 'return *"\"";'

# Preserve bytes after an embedded zero
assert 98 's: char*="a\0b"; s++; s++; return *s;'

