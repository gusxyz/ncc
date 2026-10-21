#include <stdio.h>
#include "token.h"

void print_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

int main(int argc, char**argv) {
	if(argc != 2) {
    fprintf(stderr, "Incorrect number of args\n");	
    return 1;
	}

  set_token(tokenize(argv[1]));

  print_header();
  printf("  mov rax, %d\n", expect_number());
  
  while(!at_eof()) {
    if(consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    };
    expect('-');
    printf("  sub rax, %d\n", expect_number());
  };
  printf("  ret\n");
  return 0;
}
