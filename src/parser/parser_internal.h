#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "ast/ast.h"
#include <parser/parser.h>

typedef struct Scope Scope;
typedef struct StructTag StructTag;
typedef struct Constant Constant;
typedef struct TypeAlias TypeAlias;

struct Constant {
  Constant *next;
  Token *name;
  Type *type;
  int value;
};

struct Scope {
  Scope *parent;
  LocalVariable *locals;
  Constant *constants;
  TypeAlias *types;
};

struct StructTag {
  Token *name;
  Type *type;
  StructTag *next;
};

extern Scope *current_scope;
extern int local_offset;
extern int max_local_offset;
extern Program *current_program;

struct TypeAlias {
  TypeAlias *next;
  Token *name;
  Type *type;
};

bool consume_kind(TokenKind kind);
bool is_decleration();

void enter_scope();
Node *parse_block_contents();
void parse_function_bodies(Program *program);
Constant *find_constant_in_scope(Token *name);
LocalVariable *find_lvar_in_scope(Token *name);
Node *parse_if();
Node *parse_while();
Node *parse_for();
Node *parse_decleration();
Node *parse_statement();
Node *parse_block();
Type *parse_type();
Node *expression();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *multiply();
Node *unary();
Node *postfix();
Node *primary();
Function *find_function(Token *name);

#endif