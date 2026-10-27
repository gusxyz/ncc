#include "ast/ast.h"
#include "token/token.h"
#include "type/type.h"
#include <parser/parser_internal.h>

#include <da.h>
#include <error/error.h>
#include <stdlib.h>
#include <string.h>

static Node *equality();
static Node *relational();
static Node *add();
static Node *multiply();
static Node *unary();
static Node *postfix();
static Node *primary();

Node *expression() { return assign(); }

Node *assign() {
  Node *node = logical_or();
  if (consume("=")) {
    node = new_node(NODE_ASSIGN, node, assign());
    node->type = node->left_hand_side->type;
  }
  return node;
}

static Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume("=="))
      node = new_node(NODE_EQUALS, node, relational());
    else if (consume("!="))
      node = new_node(NODE_NOT_EQUALS, node, relational());
    else
      return node;
  }
}

static Node *relational() {
  Node *node = add();
  for (;;) {
    if (consume(">"))
      node = new_node(NODE_LESS_THAN, add(), node);
    else if (consume("<"))
      node = new_node(NODE_LESS_THAN, node, add());
    else if (consume(">="))
      node = new_node(NODE_EQUAL_OR_LESS_THAN, add(), node);
    else if (consume("<="))
      node = new_node(NODE_EQUAL_OR_LESS_THAN, node, add());
    else
      return node;
  }
}

static Node *logical_and() {
  Node *node = equality();

  while(consume("&&")) {
    node = new_node(NODE_LOGICAL_AND, node, equality());
    node->type = type_int;
  }

  return node;
}

Node *logical_or() {
  Node *node = logical_and();
  
  while(consume("||")) {
    node = new_node(NODE_LOGICAL_OR, node, logical_and());
    node->type = type_int;
  }

  return node;
}

static Node *add() {
  Node *node = multiply();
  for (;;) {
    if (consume("+"))
      node = new_additive_node(NODE_ADD, node, multiply());
    else if (consume("-"))
      node = new_additive_node(NODE_SUBTRACT, node, multiply());
    else
      return node;
  }
}

static Node *multiply() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(NODE_MULTIPLY, node, unary());
    } else if (consume("/")) {
      node = new_node(NODE_DIVIDE, node, unary());
    } else
      return node;
  }
}

static Node *unary() {
  if(consume_kind(TOKEN_SIZEOF)) {
    Type *type;

    if(consume("(")) {
      if(is_type_start())
        type = parse_type();
      else
       type = expression()->type;

      expect(')');
    } else {
      type = unary()->type;
    }

    if(!type || type->size <= 0)
      error_at(global_token->string, "sizeof requires a complete type.");

    return new_node_number(type->size);
  }

  if (consume("!")) {
    Node * node = new_node(NODE_LOGICAL_NOT, unary(), NULL);
    node->type = type_int;
    return node;
  }
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_node(NODE_SUBTRACT, new_node_number(0), unary());
  if (consume("++"))
    return new_node(NODE_PRE_INCREMENT, unary(), NULL);
  if (consume("--"))
    return new_node(NODE_PRE_DECREMENT, unary(), NULL);
  if (consume("*")) {
    Node *operand = unary();

    if (!operand->type || operand->type->kind != TYPE_POINTER)
      error("Cannot dereference a non-pointer");

    Node *node = new_node(NODE_DEREFERENCE, operand, NULL);
    node->type = operand->type->base;
    return node;
  }

  if (consume("&")) {
    Node *operand = unary();

    if (operand->kind != NODE_LOCAL_VARIABLE &&
        operand->kind != NODE_GLOBAL_VARIABLE &&
        operand->kind != NODE_DEREFERENCE && operand->kind != NODE_MEMBER)
      error("Cannot take address of a non-lvalue");

    Node *node = new_node(NODE_ADDRESS, operand, NULL);
    node->type = pointer_to(operand->type);
    return node;
  }
  
  return postfix();
}

static Node *parse_index(Node *node) {
  Node *index = expression();
  expect(']');

  if (node->type->kind != TYPE_POINTER && node->type->kind != TYPE_ARRAY)
    error("Attempted to index a non pointer");

  Node *add = new_additive_node(NODE_ADD, node, index);
  Node *deref = new_node(NODE_DEREFERENCE, add, NULL);
  deref->type = add->type->base;
  return deref;
}

static Node *parse_member(Node *object) {
  if (object->type->kind != TYPE_STRUCT)
    error("Member access requires a struct");

  Token *name = consume_identifier();
  if (!name)
    error_at(global_token->string, "Expected a member name.");

  for (StructMember *member = object->type->members; member;
       member = member->next) {
    if (member->length == name->length &&
        memcmp(member->name, name->string, member->length) == 0) {
      Node *node = new_node(NODE_MEMBER, object, NULL);
      node->member = member;
      node->type = member->type;
      return node;
    }
  }

  error_at(name->string, "Unkown member of struct");
  return NULL;
}

static Node *postfix() {
  Node *node = primary();
  for (;;) {
    if (consume("++"))
      node = new_node(NODE_POST_INCREMENT, node, NULL);
    else if (consume("--"))
      node = new_node(NODE_POST_DECREMENT, node, NULL);
    else if (consume("["))
      node = parse_index(node);
    else if (consume("."))
      node = parse_member(node);
    else if (consume("->")) {
      if (node->type->kind != TYPE_POINTER ||
          node->type->base->kind != TYPE_STRUCT)
        error_at(global_token->string,
                 "Arrow access requires a pointer to a struct");

      Node *deref = new_node(NODE_DEREFERENCE, node, NULL);
      deref->type = node->type->base;

      node = parse_member(deref);
    } else
      return node;
  }
}

static Node *parse_function_call(Token *identifier_token) {
  bool is_syscall = identifier_token->length == 7 &&
                    memcmp(identifier_token->string, "syscall", 7) == 0;

  Function *function = find_function(identifier_token);

  if (!function && !is_syscall)
    error_at(identifier_token->string, "Undeclared function");

  Node *node = new_node(NODE_FUNCTION_CALL, NULL, NULL);
  node->token = identifier_token;
  node->type = is_syscall ? type_int : function->return_type;

  size_t length = identifier_token->length;
  node->function_name = calloc(length + 1, sizeof(char));
  memcpy(node->function_name, identifier_token->string, length);

  if (!consume(")")) {
    do {
      da_append(&node->arguments, assign());
    } while (consume(","));

    expect(')');
  }
  if (!is_syscall && node->arguments.count != function->paramaters.count)
    error_at(identifier_token->string, "Expected %zu arguments, got %zu",
             function->paramaters.count, node->arguments.count);
  return node;
}

static Node *primary() {
  if (consume("(")) {
    Node *node = expression();
    expect(')');
    return node;
  }

  if (global_token->kind == TOKEN_CHAR_LITERAL) {
    int value = global_token->value;
    advance_tokenizer();

    Node *node = new_node_number(value);
    node->type = type_int;
    return node;
  }

  if (global_token->kind == TOKEN_STRING_LITERAL) {
    Node *node = new_node(NODE_STRING_LITERAL, NULL, NULL);
    node->type = pointer_to(type_u8);
    node->string_length = global_token->string_length;
    node->string_value = calloc(node->string_length + 1, 1);
    memcpy(node->string_value, global_token->string_value, node->string_length);

    advance_tokenizer();
    return node;
  }

  Token *identifier_token = consume_identifier();
  if (identifier_token) {
    // its a function
    if (consume("(")) {
      return parse_function_call(identifier_token);
    }

    for (Scope *scope = current_scope; scope; scope = scope->parent) {
      for (LocalVariable *var = scope->locals; var; var = var->next) {
        if (var->length == identifier_token->length &&
            memcmp(var->name, identifier_token->string, var->length) == 0)
          return new_node_local_variable(var);
      }

      for (Constant *constant = scope->constants; constant;
           constant = constant->next) {
        if (constant->name->length == identifier_token->length &&
            memcmp(constant->name->string, identifier_token->string,
                   identifier_token->length) == 0) {
          Node *node = new_node_number(constant->value);
          node->type = constant->type;
          return node;
        }
      }
    }

    GlobalVariable *global = find_global(identifier_token);
    if (global) {
      Node *node = calloc(1, sizeof(*node));
      node->kind = NODE_GLOBAL_VARIABLE;
      node->global = global;
      node->type = global->type;
      return node;
    }

    error_at(identifier_token->string, "Undeclared variable");
  }

  return new_node_number(expect_number());
}
