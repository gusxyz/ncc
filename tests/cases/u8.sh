# u8 storage and conversion
assert_program 255 'int main() { x: u8 = 255; return x; }'
assert_program 233 'int main() { x: u8 = 233; return x; }'
assert_program 254 'int main() { x: u8 = 253; return x+1; }'

# u8 increment and decrement
assert_program 0 'int main() { x: u8 = 255; ++x; return x; }'
assert_program 255 'int main() { x: u8 = 255; old: int=x++; return old+x; }'
assert_program 255 'int main() { x: u8 = 0; --x; return x; }'
assert_program 0 'int main() { x: u8 = 0; old: int=x--; return old; }'

# Mixed local sizes: try both stack layouts
assert_program 43 'int main() { a: int=42; b: u8=1; return a+b; }'
assert_program 43 'int main() { b: u8=1; a: int=42; return a+b; }'

# u8 function parameter
assert_program 1 'int get(u8 x) { return x; } int main() { return get(257); }'

