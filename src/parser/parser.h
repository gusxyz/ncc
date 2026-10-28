#ifndef PARSER_H
#define PARSER_H

#include <ast/ast.h>
#include <stddef.h>
#include <token/token.h>
#include <type/type.h>

// Grammar
Program *parse(char *source, char *source_path);
void parse_source(Program *program, char *source_path);
void parse_function_bodies(Program *program);

#endif
