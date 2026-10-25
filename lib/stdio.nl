int linux_write(int fd, char *buffer, int length) { 
  return syscall(1, fd, buffer, length); 
}