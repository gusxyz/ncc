# System Calls
# getpid: syscall number and return value
assert 1 'return syscall(39)>0;'

# close(-1) returns raw -EBADF (-9), rather than libc's -1
assert 1 'return syscall(3,-1)==-9;'

# kill(getpid(), 0) checks two syscall arguments and a nested syscall
assert 1 'return syscall(62,syscall(39),0)==0;'

# write(fd, buffer, length) checks three arguments; prints OK
assert 3 'return syscall(1,1,"OK\n",3);'

# exit(status) should terminate with status 42
assert 42 'return syscall(60,42);'

# runtime tests
assert_program 3 'int linux_write(fd: int, buffer: char*, length: int) { return syscall(1, fd, buffer, length); } int main() { return linux_write(1, "OK\n", 3); }'

