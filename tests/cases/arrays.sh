assert 42 'values: int[3]; values[0]=10; values[1]=20; values[2]=12; return values[0]+values[1]+values[2];'
assert 42 'values: u8[3]; values[0]=10; values[1]=20; values[2]=12; return values[0]+values[1]+values[2];'
assert 42 'values: int[3]; i := 2; values[i]=42; return values[i];'
assert 1 'values: u8[2]; values[1]=42; values[0]=257; return values[0]==1 && values[1]==42;'
assert 42 'values: int[2]; p: int* = values; p[1]=42; return values[1];'
assert 42 'values: int[3]; for(i := 0; i<3; i++) values[i]=14; return values[0]+values[1]+values[2];'

assert_program 42 'int get(int *values) { return values[1]; } int main() { values: int[2]; values[1]=42; return get(values); }'