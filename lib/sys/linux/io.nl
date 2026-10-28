int linux_write(fd: int, buffer: char*, length: int) { 
  return syscall(1, fd, buffer, length); 
}