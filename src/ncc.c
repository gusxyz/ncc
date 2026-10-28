#include <import/import.h>
#include <stdio.h>
#include <token/token.h>
#include <error/error.h>
#include <parser/parser.h>
#include <codegen/codegen.h>
#include <emit/emit.h>
#include <stdbool.h>
#include <string.h>
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
  
  generate_program(parse(source, source_path));
  return 0;
}