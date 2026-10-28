int strlen(s: char*) {
  length: int = 0;
  while (s[length] != '\0') {
    length++;
  }
  return length;
} 