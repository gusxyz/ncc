#import "answer.nl"

LIMIT :: 3;
Byte :: u8;
Point :: struct { int x; int y; };

int main() {
  p: Point;
  p.x = 20;
  p.y = 22;
  values: int[3];
  for (i := 0; i < LIMIT; i++) {
    values[i] = i + 1;
  }
  byte: Byte = (u8)257;
  ptr := &values[0];
  address := (int)ptr;
  restored := (int*)address;
  size := sizeof(values);
  if (!0 && byte == 1 || size == 24) {
    syscall(1, 1, "OK\n", 3);
  }
  return p.x + p.y;
}
