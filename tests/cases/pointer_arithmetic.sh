# Pointer addition and subtraction
assert 98 'p: char*="abc"; return *(p+1);'
assert 99 'p: char*="abc"; return *(2+p);'
assert 97 'p: char*="abc"; return *((p+2)-2);'

# Pointer differences count elements
assert 3 'p: char*="abc"; return (p+3)-p;'
assert 1 'p: char*="abc"; return ((p+1)-(p+3)) == -2;'
assert 1 'x: int; p: int*=&x; return (p+1)-p;'

# Shared helper must preserve integer subtraction
assert 7 'return 10-3;'
assert 1 'return 3-5 == -2;'

# Invalid pointer arithmetic
assert_error 'Can not add two pointers together' \
  'int main() { x: int; p: int*=&x; return p+p; }'

assert_error 'Cannot subtract a pointer from an integer' \
  'int main() { x: int; p: int*=&x; return 1-p; }'

assert_error 'Cannot subtract pointers to different types' \
  'int main() { x: int; y: char; return &x-&y; }'

# Recursive type compatibility: int ** versus char **
assert_error 'Cannot subtract pointers to different types' \
  'int main() { p: int*; q: char*; return &p-&q; }'

# Pointer Indexing
assert 104 's: char*="hello"; return s[0];'
assert 101 's: char*="hello"; return s[1];'
assert 9 'x: int=1; p: int*=&x; p[0]=9; return x;'
assert 3 'x: int=1; p: int*=&x; p[0]++; return x+1;'

