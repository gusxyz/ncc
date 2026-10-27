#ifndef PARSER_H
#define PARSER_H

#include <token/token.h>
#include <stddef.h>
#include <type/type.h>
#include <ast/ast.h>

LocalVariable *find_lvar(Token *token);
LocalVariable *new_local_variable(Token *token, Type *type);

// Grammar
void parse();
void parse_source(Program *program, const char *source_path);
void parse_function_bodies(Program *program);

#endif
