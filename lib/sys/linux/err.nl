int is_error(result: int) {
  if (result >= -4095 && result <= -1)
    return result;
    
  return 0;
}
