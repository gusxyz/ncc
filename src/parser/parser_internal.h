#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include <parser/parser.h>

typedef struct Scope Scope;
typedef struct Constant Constant;
typedef struct TypeAlias TypeAlias;

struct Constant {
  Constant *next;
  Token *name;
  Type *type;
  int value;
};

struct TypeAlias {
  TypeAlias *next;
  Token *name;
  Type *type;
};

struct Scope {
  Scope *parent;
  LocalVariable *locals;
  Constant *constants;
  TypeAlias *types;
};

extern Scope *current_scope;
extern Scope global_scope;
extern int local_offset;
extern int max_local_offset;
extern Program *current_program;

// Parser state and program symbols
bool consume_kind(TokenKind kind);
GlobalVariable *find_global(Token *token);
Function *find_function(Token *name);

// Scopes and local symbols
void enter_scope();
LocalVariable *find_lvar_in_scope(Token *token);
LocalVariable *find_lvar(Token *token);
Constant *find_constant_in_scope(Token *name);
Constant *find_constant(Token *token);
TypeAlias *find_type_alias(Token *name);
TypeAlias *find_type_alias_in_current_scope(Token *name);
LocalVariable *new_local_variable(Token *token, Type *type);

// Types and declarations
bool is_decleration();
Type *parse_type();
Node *parse_decleration();
bool is_type_start();

// Statements
Node *parse_statement();
Node *parse_block_contents();

// Expressions
Node *expression();
Node *assign();
Node *logical_or();

#endif
