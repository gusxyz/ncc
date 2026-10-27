#ifndef PARSER_H
#define PARSER_H

#include <ast/ast.h>
#include <stddef.h>
#include <token/token.h>
#include <type/type.h>

// Grammar
void parse();
void parse_source(Program *program, const char *source_path);
void parse_function_bodies(Program *program);

#endif
