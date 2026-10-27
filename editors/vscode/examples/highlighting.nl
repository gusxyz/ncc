#import "answer.nl"

LIMIT :: 3;
Byte :: u8;
Point :: struct { int x; int y; };

int main() {
  point: Point;
  point.x = 20;
  point.y = 22;
  values: int[3];
  for (i := 0; i < LIMIT; i++) {
    values[i] = i + 1;
  }
  byte: Byte = (Byte)257;
  ptr := &values[0];
  address := (int)ptr;
  restored := (int*)address;
  size := sizeof(values);
  x := 0;
  0 && (x = 1);
  1 || (x = 2);
  if (!(byte == 1 && size == 24 && x == 0)) return 1;
  if (restored[2] != 3) return 2;
  if (answer() != 42) return 3;
  text: char* = "OK\n";
  newline := '\n';
  if (text[2] != newline) return 4;
  return point.x + point.y;
}
