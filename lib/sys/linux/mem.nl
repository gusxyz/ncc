PROT_READ     :: 1;
PROT_WRITE    :: 2;

MAP_PRIVATE   :: 2;
MAP_ANONYMOUS :: 32;

void* mmap(addr: void*, length: int, prot: int, flags: int, fd: int, offset: int) {
  return (void*) syscall(9, addr, length, prot, flags, fd, offset);
}

int munmap(addr: void*, length: int) {
  return syscall(11, addr, length);
}