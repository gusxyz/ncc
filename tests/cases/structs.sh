# Basic member reads and writes
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; p.x=20; p.y=22; return p.x+p.y; }'

# Members retain independent values
assert_program 42 'struct Pair { int a; int b; }; int main() { p: struct Pair; p.a=10; p.b=32; p.a=p.a+1; return p.a+p.b-1; }'

# Mixed widths and alignment
assert_program 42 'struct Mixed { char a; int b; char c; }; int main() { m: struct Mixed; m.a=1; m.b=39; m.c=2; return m.a+m.b+m.c; }'

# Byte stores must not overwrite neighboring fields
assert_program 42 'struct Bytes { u8 a; u8 b; }; int main() { v: struct Bytes; v.b=42; v.a=257; if(v.a!=1) return 0; return v.b; }'

# Nested structs
assert_program 42 'struct Inner { int x; }; struct Outer { char tag; struct Inner inner; int y; }; int main() { o: struct Outer; o.tag=1; o.inner.x=20; o.y=21; return o.tag+o.inner.x+o.y; }'

# Address of a member
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; x: int*=&p.x; *x=42; return p.x; }'

# Arrow access
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; ptr: struct Point*=&p; ptr->x=20; ptr->y=22; return p.x+p.y; }'

# Struct pointers passed to functions
assert_program 42 'struct Point { int x; }; int set(struct Point *p) { p->x=42; return 0; } int main() { p: struct Point; set(&p); return p.x; }'

# Pointer-valued members
assert_program 42 'struct Holder { int *value; }; int main() { x: int=42; h: struct Holder; h.value=&x; return *h.value; }'

# Self-referencing struct type and chained access
assert_program 42 'struct Node { int value; struct Node *next; }; int main() { a: struct Node; b: struct Node; a.next=&b; b.value=42; return a.next->value; }'

# Global struct storage
assert_program 42 'struct Point { int x; int y; }; struct Point p; int main() { p.x=20; p.y=22; return p.x+p.y; }'

# Prefix and postfix operations on members
assert_program 42 'struct Counter { int value; }; int main() { c: struct Counter; c.value=41; return ++c.value; }'
assert_program 41 'struct Counter { int value; }; int main() { c: struct Counter; c.value=20; old: int=c.value++; return old+c.value; }'

# Struct test errors
assert_error 'Unkown member of struct' \
  'struct Point { int x; }; int main() { p: struct Point; return p.y; }'

assert_error 'Member access requires a struct' \
  'int main() { x: int=1; return x.y; }'

assert_error 'Arrow access requires a pointer to a struct' \
  'struct Point { int x; }; int main() { p: struct Point; return p->x; }'

assert_error 'Duplicate member name' \
  'struct Point { int x; int x; }; int main() { return 0; }'

assert_error 'Member requires a complete type' \
  'struct Node { struct Node next; }; int main() { return 0; }'

