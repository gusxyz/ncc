#include <parser/parser_internal.h>

Scope *current_scope;
int local_offset;
int max_local_offset;

void enter_scope() {
  Scope *scope = calloc(1,sizeof(*scope));
  scope->parent = current_scope;
  current_scope = scope;
}

