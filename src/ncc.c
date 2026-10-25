#include "import.h"
#include <stdio.h>
#include <token.h>
#include <error.h>
#include <parser.h>
#include <codegen.h>
#include <emit.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

int main(int argc, char**argv) {
  char *source;
  char *source_path;
	if(argc == 3 && strcmp(argv[1], "-e") == 0) {
    source = argv[2];
    source_path = NULL;
  } else if (argc == 2) {
    source = read_source_file(argv[1]);
    source_path = argv[1];
  } else {
    fprintf(stderr, "Usage: ncc -e 'source' | ncc file.nl\n");	
    return 1;
	}

  Program *prog = calloc(1, sizeof(*prog));

  if(source_path)
    mark_source_loaded(source_path);

  set_user_input(source);
  set_token(tokenize(source));
  parse_source(prog, source_path);

  for (size_t i = 0; i < prog->imports.count; i++) {
    ImportRequest request = prog->imports.items[i];

    char *path = resolve_import_path(request.from_path, request.path);

    if(!mark_source_loaded(path)) {
      free(path);
      continue;
    }

    char *imported_source = read_source_file(path);

    set_user_input(imported_source);
    set_token(tokenize(imported_source));

    parse_source(prog, path);
  }

  parse_function_bodies(prog);

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