assert_program 1 'Color :: enum { RED, GREEN, BLUE }; int main() { return Color.GREEN; }'
assert_program 2 'Color :: enum { RED, GREEN, BLUE, }; int main() { return Color.BLUE; }'
assert_error 'Unknown enum member' 'Color :: enum { RED }; int main() { return Color.BLUE; }'
assert_error 'Member with that name already exists' 'Color :: enum { RED, RED }; int main() { return 0; }'