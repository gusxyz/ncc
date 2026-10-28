#include "ast/ast.h"
#include <token/token.h>
#include <type/type.h>
#include <parser/parser_internal.h>

#include <da.h>
#include <error/error.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct StructTag StructTag;

struct StructTag {
  Token *name;
  Type *type;
  StructTag *next;
};

static StructTag *struct_tags;

static StructTag *find_struct_tag(Token *name) {
  for (StructTag *tag = struct_tags; tag; tag = tag->next) {
    if (tag->name->length == name->length &&
        memcmp(tag->name->string, name->string, name->length) == 0)
      return tag;
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
  switch (global_token->kind) {
  case TOKEN_INT:
  case TOKEN_U8:
  case TOKEN_CHAR:
  case TOKEN_STRUCT:
  case TOKEN_ENUM:
  case TOKEN_VOID:
    return true;
  case TOKEN_IDENTIFIER:
    return find_type_alias(global_token);
  default:
    return false;
  }
}

static void allocate_struct_member(Type *type, StructMember ***tail,
                                   int *offset, Type *member_type,
                                   Token *member_name) {
  StructMember *member = calloc(1, sizeof(*member));
  member->name = calloc(member_name->length + 1, 1);
  memcpy(member->name, member_name->string, member_name->length);
  member->length = member_name->length;
  member->type = member_type;

  int alignment = member_type->alignment;
  (*offset) = ((*offset) + alignment - 1) / alignment * alignment;

  member->offset = (*offset);
  (*offset) += member_type->size;

  if (alignment > type->alignment)
    type->alignment = alignment;

  *(*tail) = member;
  (*tail) = &member->next;
}

static void parse_struct_body(Type *type) {
  StructMember **tail = &type->struct_members;
  int offset = 0;

  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of struct");

    Type *member_type = parse_type();
    Token *member_name = consume_identifier();

    if (!member_name)
      error_at(global_token->string, "Expected a member name");
    
    if (member_type->kind == TYPE_VOID)
      error_at(member_name->string, "Struct member cannot have type void");

    expect(';');

    for (StructMember *m = type->struct_members; m; m = m->next) {
      if (m->length == member_name->length &&
          memcmp(m->name, member_name->string, member_name->length) == 0)
        error_at(member_name->string, "Duplicate member name");
    }

    if (member_type->size == 0)
      error_at(member_name->string, "Member requires a complete type");
    allocate_struct_member(type, &tail, &offset, member_type, member_name);
  }

  if (!type->struct_members)
    error_at(global_token->string,
             "Declaring a struct without members is not supported");

  type->size =
      (offset + type->alignment - 1) / type->alignment * type->alignment;
}

static Type *parse_struct_type() {
  Token *name = consume_identifier();
  StructTag *tag = name ? find_struct_tag(name) : NULL;

  if (name)
    tag = find_struct_tag(name);

  if (!consume("{")) {
    if (!name)
      error_at(global_token->string, "expected a struct name or '{'");
    if (!tag)
      error_at(name->string, "Unknown struct type");

    return tag->type;
  }

  if (name && current_scope && current_scope != &global_scope)
    error_at(name->string, "Struct definitions require top-level scope");
  if (tag)
    error_at(name->string, "Struct with this name already defined.");

  Type *type = calloc(1, sizeof(*type));
  type->kind = TYPE_STRUCT;
  type->alignment = 1;

  if (name) {
    tag = calloc(1, sizeof(*tag));
    tag->name = name;
    tag->type = type;
    tag->next = struct_tags;
    struct_tags = tag;
  }

  parse_struct_body(type);
  return type;
}

void parse_enum_members(Type *type) {
  EnumMember **tail = &type->enum_members;
  int value = 0;
  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of enum");

    Token *name = consume_identifier();
    if (!name)
      error_at(global_token->string, "Expected an enum member name");

    for (EnumMember *enum_member = type->enum_members; enum_member;enum_member = enum_member->next)
      if (memcmp(enum_member->name, name->string, name->length) == 0)
        error_at(name->string, "Member with that name already exists");

    EnumMember *member = calloc(1, sizeof(*member));
    member->name = name->string;
    member->length = name->length;
    member->value = value;

    *tail = member;
    tail = &member->next;

    if (consume("}"))
      break;

    expect(',');

    if (consume("}"))
      break;

    if (value == INT_MAX)
      error_at(global_token->string, "Enum value overflow");
    
    value++;
  }
}

Type *parse_enum() {
  expect('{');

  Type *type = calloc(1, sizeof(*type));
  type->kind = TYPE_ENUM;
  type->size = 8;
  type->alignment = 8;

  parse_enum_members(type);
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
  else if (consume_kind(TOKEN_VOID))
    type = type_void;
  else if (consume_kind(TOKEN_STRUCT))
    type = parse_struct_type();
  else if (consume_kind(TOKEN_ENUM))
    type = parse_enum();
  else {
    TypeAlias *alias = NULL;

    if (global_token->kind == TOKEN_IDENTIFIER)
      alias = find_type_alias(global_token);

    if (!alias)
      error_at(global_token->string, "Expected a type");
    else {
      type = alias->type;
      advance_tokenizer();
    }
  }

  while (consume("*"))
    type = pointer_to(type);

  while (consume("[")){
    int length = expect_number();
    expect(']');
    type = array_of(type, length);
  }

  return type;
}

Node *parse_decleration() {
  Token *name = consume_identifier();
  if (!name)
    error_at(global_token->string, "Expected a variable name");

  if (find_lvar_in_scope(name) || find_constant_in_scope(name) ||
      find_type_alias_in_current_scope(name))
    error_at(global_token->string, "Variable already declared");

  Type *type;
  Node *value = NULL;
  LocalVariable *variable;

  if (consume(":=")) {
    value = assign();
    require_value(value);
    type = value->type;

    if (!type)
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

    if (consume("=")) {
      value = assign();
      require_value(value);
    }
  }

  Node *block = new_node(NODE_BLOCK, NULL, NULL);

  if (value) {
    Node *init =
        new_node(NODE_ASSIGN, new_node_local_variable(variable), value);

    init->type = type;
    da_append(&block->body, init);
  }

  expect(';');
  return block;
}
