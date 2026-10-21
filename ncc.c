#include <stdio.h>
#include "token.h"
#include "error.h"
#include "parser.h"

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

  set_user_input(argv[1]);
  set_token(tokenize(argv[1]));
  Node *node = expression();

  print_header();
  generate_code(node);

  if (!at_eof())
    error("Unexpected token after expression");

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
