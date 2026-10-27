Point :: struct { int x; int y; };

int main() {
  p: Point;
  p.x = 20;
  p.y = 22;
  return p.x + p.y;
}