int print(str: char*) {
  return is_error(linux_write(1, str, strlen(str)));
}

#import "sys/linux/io.nl"
#import "sys/linux/err.nl"
#import "stdstr.nl"