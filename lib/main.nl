arena : Arena;

int main() {
  arena.bytePointer = (u8*) 0;
  arena.bytesUsed = 0;
  arena.capacity = 0;

  success := arena_init(&arena, 4096);

  if(success) {
    print("Arena init succesfully\n");
    return is_error(munmap((void*) arena.bytePointer, arena.capacity));
  } else {
    print("Arena init failed\n");
    return 1;
  }

  arena.bytePointer[0] = 5;
  
  /*
  1. Write a byte through the pointer, or index it to write at a particular byte offset.
  2. Read that same location back to verify the value.
  3. Keep every access within the arena’s capacity.
  */
  return 1;
}

#import "stdio.nl"
#import "stdmem.nl"
#import "sys/linux/err.nl"
#import "sys/linux/mem.nl"
