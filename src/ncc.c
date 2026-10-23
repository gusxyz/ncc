#include <stdio.h>
#include <token.h>
#include <error.h>
#include <parser.h>
#include <codegen.h>
#include <emit.h>
#include <stdbool.h>
#include <string.h>

int main(int argc, char**argv) {
	if(argc != 2) {
    fprintf(stderr, "Incorrect number of args\n");	
    return 1;
	}

  set_user_input(argv[1]);
  set_token(tokenize(argv[1]));

  Program *prog = parse_program();
  bool has_main = false;

  for (size_t i = 0; i < prog->functions.count; i++) {
    Function *function = prog->functions.items[i];

    if (strcmp(function->name, "main") == 0)
      has_main = true;
  }

  if(!has_main)
    error("No defined main function");

  generate_program(prog);
  return 0;
}
