#include "token/token.h"
#include <parser/parser_internal.h>
#include <string.h>
#include <error/error.h>
#include <da.h>

static StructTag *struct_tags;

static StructTag *find_struct_tag(Token *name) {
  for(StructTag *tag = struct_tags; tag; tag = tag->next) {
    if (tag->name->length == name->length &&
        memcmp(tag->name->string, name->string, name->length) == 0)
        return tag;
  }

  return NULL;
}

static int evaluate_constant(Node *node) {
  switch (node->kind) {
  case NODE_NUMBER:
    return node->value;
  case NODE_ADD:
    return evaluate_constant(node->left_hand_side) +
           evaluate_constant(node->right_hand_side);
  case NODE_SUBTRACT:
    return evaluate_constant(node->left_hand_side) -
           evaluate_constant(node->right_hand_side);
  case NODE_MULTIPLY:
    return evaluate_constant(node->left_hand_side) *
           evaluate_constant(node->right_hand_side);
  case NODE_DIVIDE: {
    int left = evaluate_constant(node->left_hand_side);
    int right = evaluate_constant(node->right_hand_side);

    if (right == 0)
      error("Division by zero in constant expression");

    return left / right;
  }

  default:
    error("Expected a constant integer expression");
    return 0;
  }
}

static TypeAlias *find_type_alias(Token *name) {
  for (Scope *scope = current_scope; scope; scope = scope->parent) {
    for (TypeAlias *alias = scope->types; alias; alias = alias->next) {
      if (alias->name->length == name->length &&
          memcmp(alias->name->string, name->string, name->length) == 0)
        return alias;
    }
  }
  return NULL;
}

static TypeAlias *find_type_alias_in_current_scope(Token *name) {
  for (TypeAlias *alias = current_scope->types; alias; alias = alias->next) {
    if (alias->name->length == name->length &&
        memcmp(alias->name->string, name->string, name->length) == 0)
      return alias;
  }
  return NULL;
}

bool is_decleration() {
  if (global_token->kind != TOKEN_IDENTIFIER)
    return false;

  Token *next = global_token->next_token;
  if (!next || next->kind != TOKEN_RESERVED)
    return false;

  return (next->length == 1 && next->string[0] == ':') ||
         (next->length == 2 && memcmp(next->string, ":=", 2) == 0) ||
         (next->length == 2 && memcmp(next->string, "::", 2) == 0);
}

bool is_type_start() {
  switch(global_token->kind) {
    case TOKEN_INT:
    case TOKEN_U8:
    case TOKEN_CHAR:
    case TOKEN_STRUCT:
      return true;
    case TOKEN_IDENTIFIER:
      return find_type_alias(global_token);
    default:
      return false;
  }
}

Type *parse_type();

static Type *parse_struct_type() {
  Token *name = consume_identifier();
  if(!name)
    error_at(global_token->string, "Expected a struct name");

  StructTag *tag = find_struct_tag(name);
  if(!consume("{")) {
    if(!tag)
      error_at(name->string, "Unkown struct type");
    return tag->type;
  }

  if (current_scope)
    error_at(name->string, "Struct definitions require top-levle scope");

  if (tag)
    error_at(name->string, "Struct with this name already defined.");

  Type *type = calloc(1, sizeof(*type));
  type->kind = TYPE_STRUCT;
  type->alignment = 1;

  tag = calloc(1, sizeof(*tag));
  tag->name = name;
  tag->type = type;
  tag->next = struct_tags;
  struct_tags = tag;

  StructMember **tail = &type->members;
  int offset = 0;

  while (!consume("}")) {
    if(at_eof())
      error("Expected '} before end of struct");

    Type *member_type = parse_type();
    Token *member_name = consume_identifier();

    if(!member_name)
      error_at(global_token->string, "Expected a member name");

    expect(';');

    for(StructMember *m = type->members; m; m = m->next) {
      if(m->length == member_name->length &&
        memcmp(m->name, member_name->string, member_name->length) == 0)
        error_at(member_name->string, "Duplicate member name");
    }

    if(member_type->size == 0)
      error_at(member_name->string, "Member requires a complete type");

    StructMember *member = calloc(1, sizeof(*member));
    member->name = calloc(member_name->length + 1, 1);
    memcpy(member->name, member_name->string, member_name->length);
    member->length = member_name->length;
    member->type = member_type;

    int alignment = member_type->alignment;
    offset = (offset + alignment - 1) / alignment * alignment;

    member->offset = offset;
    offset += member_type->size;
    
    if(alignment > type->alignment)
      type->alignment = alignment;

    *tail = member;
    tail = &member->next;
  }

  if(!type->members)
    error_at(name->string, "Declareing a struct without members is not supported");

  type->size = (offset + type->alignment - 1) / type->alignment * type->alignment;
  return type;
}

Type *parse_type() {
  Type *type;

  if (consume_kind(TOKEN_INT))
    type = type_int;
  else if (consume_kind(TOKEN_U8))
    type = type_u8;
  else if (consume_kind(TOKEN_CHAR))
    type = type_u8;
  else if(consume_kind(TOKEN_STRUCT))
    type = parse_struct_type();
  else {
    TypeAlias *alias = NULL;

    if (global_token->kind == TOKEN_IDENTIFIER)
      alias = find_type_alias(global_token);

    if(!alias)
      error_at(global_token->string, "Expected a type");
    else {
      type = alias->type;
      advance_tokenizer();
    }
  }

  while (consume("*"))
    type = pointer_to(type);

  return type;
}

void parse_function_bodies(Program *program) {
  current_program = program;

  for (size_t i = 0; i < program->functions.count; i++) {
    Function *function = program->functions.items[i];

    set_user_input(function->source_text);
    set_token(function->body_tokens);

    current_scope = NULL;
    enter_scope();

    current_scope->locals = function->locals;

    local_offset = function->locals ? function->locals->offset : 0;
    max_local_offset = local_offset;

    function->body = parse_block_contents();
    function->locals = current_scope->locals;
    function->stack_size = (max_local_offset + 15) / 16 * 16;

    current_scope = NULL;
  }
}

Function *find_function(Token *name) {
  for (size_t i = 0; i < current_program->functions.count; i++) {
    Function *function = current_program->functions.items[i];
    if (strlen(function->name) == name->length &&
        memcmp(function->name, name->string, name->length) == 0)
        return function;
  }
  return NULL;
}

Node *parse_decleration() { 
  Token *name = consume_identifier();
  if(!name)
    error_at(global_token->string, "Expected a variable name");
    
  if(find_lvar_in_scope(name) || find_constant_in_scope(name) || find_type_alias_in_current_scope(name))
    error_at(global_token->string, "Variable already declared");

  Type *type;
  Node *value = NULL;
  LocalVariable *variable;

  if(consume(":=")) {
    value = assign();
    type = value->type;

    if(!type)
      error_at(name->string, "Cannot infer initializer type");
    variable = new_local_variable(name, type);
  } else if (consume("::")) {
    if (is_type_start()) {
      Type *type = parse_type();

      TypeAlias *alias = calloc(1, sizeof(*alias));
      alias->name = name;
      alias->type = type;
      alias->next = current_scope->types;
      current_scope->types = alias;
    } else {
      Node *initializer = assign();

      Constant *constant = calloc(1, sizeof(*constant));
      constant->name = name;
      constant->type = type_int;
      constant->value = evaluate_constant(initializer);
      constant->next = current_scope->constants;
      current_scope->constants = constant;
    }

    expect(';');
    return new_node(NODE_BLOCK, NULL, NULL);
  } else {
    expect(':');
    type = parse_type();
    variable = new_local_variable(name, type);

    if(consume("="))
      value = assign();
  }

  Node *block = new_node(NODE_BLOCK, NULL, NULL);
  
  if(value) {
    Node *init = new_node(NODE_ASSIGN, 
                new_node_local_variable(variable),
               value);

    init->type = type;
    da_append(&block->body, init);
  }

  expect(';');
  return block;

}