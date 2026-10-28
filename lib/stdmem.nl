Arena :: struct {
  u8 *bytePointer;
  int bytesUsed;
  int capacity;
};

int arena_init(arena: Arena*, size: int) {
  if(size <= 0)
    return 0;

  result := mmap((void*) 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  // check for syscall errors
  if (result >= -4095 && result <= -1)
    return 0;

  arena->bytePointer = (u8*) result;
  arena->bytesUsed = 0;
  arena->capacity = size;
  return 1;
}

int arena_alloc(arena: Arena*, size: int) {

  // take the address at bytePointer plus bytesUsed, reserve the requested number of bytes, then advance bytesUsed. Reject requests larger than the remaining capacity
  // when storing structs or integers, align the allocation address for that type before reserving space. byte writes need no extra alignment.
}
#import "sys/linux/mem.nl"
#import "sys/linux/err.nl"