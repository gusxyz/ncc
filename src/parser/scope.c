#include <parser/parser_internal.h>

#include <stdlib.h>
#include <string.h>
#include <error/error.h>

Scope *current_scope;
Scope global_scope;
int local_offset;
int max_local_offset;

void enter_scope() {
  Scope *scope = calloc(1, sizeof(*scope));
  scope->parent = current_scope;
  current_scope = scope;
}

LocalVariable *find_lvar_in_scope(Token *token) {
  for (LocalVariable *var = current_scope->locals; var; var = var->next) {
    if (var->length == token->length &&
        !memcmp(token->string, var->name, var->length))
      return var;
  }
  return NULL;
}

LocalVariable *find_lvar(Token *token) {
  for (Scope *scope = current_scope; scope; scope = scope->parent)
    for (LocalVariable *var = scope->locals; var; var = var->next)
      if (var->length == token->length &&
          !memcmp(token->string, var->name, var->length))
        return var;
  return NULL;
}

Constant *find_constant_in_scope(Token *name) {
  for (Constant *constt = current_scope->constants; constt;
       constt = constt->next) {
    if (constt->name->length == name->length &&
        memcmp(constt->name->string, name->string, name->length) == 0)
      return constt;
  }
  return NULL;
}

Constant *find_constant(Token *token) {
  for (Scope *scope = current_scope; scope; scope = scope->parent)
    for (Constant *constt = scope->constants; constt; constt = constt->next)
      if (constt->name->length == token->length &&
          !memcmp(token->string, constt->name, constt->name->length))
        return constt;
  return NULL;
}

TypeAlias *find_type_alias(Token *name) {
  for (Scope *scope = current_scope; scope; scope = scope->parent) {
    for (TypeAlias *alias = scope->types; alias; alias = alias->next) {
      if (alias->name->length == name->length &&
          memcmp(alias->name->string, name->string, name->length) == 0)
        return alias;
    }
  }
  return NULL;
}

TypeAlias *find_type_alias_in_current_scope(Token *name) {
  for (TypeAlias *alias = current_scope->types; alias; alias = alias->next) {
    if (alias->name->length == name->length &&
        memcmp(alias->name->string, name->string, name->length) == 0)
      return alias;
  }
  return NULL;
}

LocalVariable *new_local_variable(Token *token, Type *type) {
  LocalVariable *local_var = calloc(1, sizeof(LocalVariable));
  if (type->kind == TYPE_VOID)
    error_at(token->string, "Variable or parameter cannot have type void");
  local_var->name = token->string;
  local_var->length = token->length;
  local_var->type = type;

  int end = local_offset + type->size;

  local_var->offset =
      (end + type->alignment - 1) / type->alignment * type->alignment;

  local_offset = local_var->offset;
  if (local_offset > max_local_offset)
    max_local_offset = local_offset;

  local_var->next = current_scope->locals;
  current_scope->locals = local_var;

  return local_var;
}
