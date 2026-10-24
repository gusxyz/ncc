#include "type.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <parser.h>
#include <sys/types.h>
#include <token.h>
#include <da.h>
#include <error.h>

typedef struct Scope Scope;

struct Scope {
  Scope *parent;
  LocalVariable *locals;
};

static Scope *current_scope;
static int local_offset;
static int max_local_offset;

static void enter_scope() {
  Scope *scope = calloc(1,sizeof(*scope));
  scope->parent = current_scope;
  current_scope = scope;
}

Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->left_hand_side = left_hand_side;
  node->right_hand_side = right_hand_side;
  return node;
}

Node *new_node_number(int value) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NODE_NUMBER;
  node->value = value;
  node->type = type_int;
  return node;
}

bool consume_kind(TokenKind kind) {
  if (global_token->kind != kind)
    return false;
  advance_tokenizer();
  return true;
}

bool is_type_start() {
  if(global_token->kind == TOKEN_INT ||
    global_token->kind == TOKEN_U8 ||
    global_token->kind == TOKEN_CHAR)
    return true;

  return false;
}

Type *parse_type() {
  Type *type;

  if (consume_kind(TOKEN_INT))
    type = type_int;
  else if (consume_kind(TOKEN_U8))
    type = type_u8;
  else if (consume_kind(TOKEN_CHAR))
    type = type_u8;
  else
    error_at(global_token->string, "Expected a type");

  while (consume("*"))
    type = pointer_to(type);

  return type;
}

static LocalVariable *find_lvar_in_scope(Token *token) {
  for(LocalVariable *var = current_scope->locals; var; var = var->next) {
    if(var->length == token->length && !memcmp(token->string, var->name, var->length))
      return var;
  }
  return NULL;
}

LocalVariable *find_lvar(Token* token) {
  for(Scope *scope = current_scope; scope; scope = scope->parent)
    for (LocalVariable *var = scope->locals; var; var = var->next) 
      if(var->length == token->length && !memcmp(token->string, var->name, var->length))
        return var;
  return NULL;
}

LocalVariable *new_local_variable(Token *token, Type *type) {
    LocalVariable *local_var = calloc(1, sizeof(LocalVariable));

    local_var->name = token->string;
    local_var->length = token->length;
    local_var->type = type;

    int end = local_offset + type->size;

    local_var->offset = (end + type->alignment - 1) / type->alignment * type->alignment;

    local_offset = local_var->offset;
    if(local_offset > max_local_offset)
      max_local_offset = local_offset;

    local_var->next = current_scope->locals;
    current_scope->locals = local_var;

    return local_var;
}

Node *new_node_local_variable(LocalVariable *local_var) {
    Node *node = calloc(1, sizeof(*node));
    node->kind = NODE_LOCAL_VARIABLE;
    node->offset = local_var->offset;
    node->type = local_var->type;
    return node;
}

Function *parse_function() {
  Type *type = parse_type();

  Token *name = consume_identifier();
  if (!name)
    error_at(global_token->string, "Expected a function name");

  Function *function = calloc(1, sizeof(Function));
  function->return_type = type;

  function->name = calloc(name->length + 1, sizeof(char));
  memcpy(function->name, name->string, name->length);

  current_scope = NULL;
  local_offset = 0;
  max_local_offset =0 ;
  enter_scope();
    
  expect('(');

  if(!consume(")")) {
    do {
      Type *parameter_type = parse_type();
    
      Token *param_name = consume_identifier();
      if(!param_name)
        error_at(global_token->string, "Expected an argument name");
      if(find_lvar_in_scope(param_name))
        error_at(global_token->string, "Duplicate argument name");
      if(function->paramaters.count >= 6)
        error_at(param_name->string, "At most 6 paramaters allowed");

      LocalVariable *argument = new_local_variable(param_name, parameter_type);
      da_append(&function->paramaters, argument);
    } while(consume(","));
    expect(')');
  }

  function->body = parse_block_contents();
  function->stack_size = (max_local_offset + 15) / 16 * 16;

  return function;
}

Program *parse_program() {
  Program *program = calloc(1, sizeof(Program));

  while (!at_eof()) {
    da_append(&program->functions, parse_function());
  }

  return program;
}

Node *parse_if() {
  Node *node = new_node(NODE_IF, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = statement();

  if(consume_kind(TOKEN_ELSE))
      node->els = statement(); 
  return node;
}

Node *parse_while() {
  Node *node = new_node(NODE_WHILE, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = statement();

  return node;
}

static Node *parse_block_contents() {
  Node *node = new_node(NODE_BLOCK, NULL, NULL);
  expect('{');

  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of input");
    da_append(&node->body, statement());
  }

  return node;
}

Node *parse_block() {

  Scope *parent = current_scope;
  enter_scope();

  Node *node = parse_block_contents();
  
  current_scope = parent;
  return node;
}

Node *parse_for() {
    Node *node = new_node(NODE_FOR, NULL, NULL);
    expect('(');
    if (!consume(";")) {
        node->init = expression();
        expect(';');
    }
    if (!consume(";")) {
        node->condition = expression();
        expect(';');
    }
    if (!consume(")")) {
        node->inc = expression();
        expect(')');
    }
    node->then = statement();
    return node;
}

Node *parse_decleration(Type *type) { 
  Token *name = consume_identifier();
  
  if(!name)
    error_at(global_token->string, "Expected a variable name");

  if(find_lvar_in_scope(name))
    error_at(global_token->string, "Variable already declared");

  LocalVariable *variable = new_local_variable(name, type);

  Node *block = new_node(NODE_BLOCK, NULL, NULL);
  
  if(consume("=")) {
    Node *left = new_node_local_variable(variable);
    Node *right = assign();

    Node *init = new_node(NODE_ASSIGN, left, right);
    da_append(&block->body, init);
  }

  expect(';');
  return block;
}

Node *statement() {
  Node *node;

  if(is_type_start())
    return parse_decleration(parse_type());
  if (consume_kind(TOKEN_IF))
    return parse_if();
  if (consume_kind(TOKEN_WHILE))
    return parse_while();
  if (consume_kind(TOKEN_FOR))
    return parse_for();
  if (consume_kind(TOKEN_BREAK)) {
    expect(';');
    return new_node(NODE_BREAK, NULL, NULL);
  }
  if (consume_kind(TOKEN_CONTINUE)) {
    expect(';');
    return new_node(NODE_CONTINUE, NULL, NULL);
  }

  if (global_token->kind == TOKEN_RESERVED && global_token->string[0] == '{')
    return parse_block();

  if (consume_kind(TOKEN_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = NODE_RETURN;
    node->left_hand_side = expression();
  } else {
    node = expression();
  }

  if (!consume(";"))
    error_at(global_token->string, "Expected a ;");

  return node;
}

Node *expression() { return assign(); }

Node *assign() {
  Node *node = equality();
  if(consume("=")) {
    node = new_node(NODE_ASSIGN, node, assign());
    node->type = node->left_hand_side->type;
  }
  return node;
}

Node *equality() {
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

Node *relational() {
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

Node *new_additive_node(NodeKind kind, Node *left, Node *right) {
  TypeKind left_kind = left->type->kind;
  TypeKind right_kind = right->type->kind;
  bool left_integer = left_kind == TYPE_INT || left_kind == TYPE_U8;
  bool right_integer = right_kind == TYPE_INT || right_kind == TYPE_U8;
  bool left_pointer = left_kind == TYPE_POINTER;
  bool right_pointer = right_kind == TYPE_POINTER;

  if (left_integer && right_integer) {
    Node *node = new_node(kind, left, right);
    node->type = type_int;
    return node;
  }

  if(left_pointer && right_integer) {
    Node *scaled = new_node(NODE_MULTIPLY, right, new_node_number(left->type->base->size));
    scaled->type = type_int;

    Node *node = new_node(kind, left, scaled);
    node->type = left->type;
    return node;
  }

  if (left_integer && right_pointer) {
    if (kind == NODE_SUBTRACT)
      error("Cannot subtract a pointer from an integer");

    return new_additive_node(kind, right, left);
  }

  if(left_pointer && right_pointer) {
    if(kind == NODE_ADD)
        error("Can not add two pointers together");

    if (!types_equal(left->type->base, right->type->base))
      error("Cannot subtract pointers to different types");
    Node *diff = new_node(NODE_SUBTRACT, left, right);
    diff->type = type_int;

    Node *node = new_node(NODE_DIVIDE, diff, new_node_number(left->type->base->size));
    node->type = type_int;
    return node;
  }

  error("Invalid operands for additive expression");
  return NULL;
}

Node *add() {
  Node *node = multiply();
  for (;;) {
    if (consume("+"))
      node = new_additive_node(NODE_ADD, node, multiply());
    else if(consume("-"))
      node = new_additive_node(NODE_SUBTRACT, node, multiply());
    else
      return node;
  }
}

Node *multiply() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(NODE_MULTIPLY, node, unary());
      node->type = type_int;
    }
    else if (consume("/")) {
      node = new_node(NODE_DIVIDE, node, unary());
      node->type = type_int;
    }
    else
      return node;
  }
}

Node *unary() {
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
        operand->kind != NODE_DEREFERENCE)
      error("Cannot take address of a non-lvalue");

    Node *node = new_node(NODE_ADDRESS, operand, NULL);
    node->type = pointer_to(operand->type);
    return node;
  }
  return postfix();
}

Node *parse_index(Node *node) {
  Node *index = expression();
  expect(']');

  if(node->type->kind != TYPE_POINTER)
    error("Attempted to index a non pointer");  

  Node *add = new_additive_node(NODE_ADD, node, index);
  Node *deref = new_node(NODE_DEREFERENCE, add, NULL);
  deref->type = add->type->base;
  return deref;
}

Node *postfix() {
    Node *node = primary();
    for(;;) {  
      if(consume("++"))
        node = new_node(NODE_POST_INCREMENT, node, NULL);
      else if(consume("--"))
        node = new_node(NODE_POST_DECREMENT, node, NULL);
      else if(consume("[")) {
        node = parse_index(node);
      } else
        return node;
    }
}

Node *primary() {
    if (consume("(")) {
        Node *node = expression();
        expect(')');
        return node;
    }

    if(global_token->kind == TOKEN_CHAR_LITERAL) {
        int value = global_token->value;
        advance_tokenizer();

        Node* node = new_node_number(value);
        node->type = type_int;
        return node;
    }

    if(global_token->kind == TOKEN_STRING_LITERAL) {
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
      if(consume("(")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = NODE_FUNCTION_CALL;
        node->type = type_int;

        size_t length = identifier_token->length;
        node->function_name = calloc(length +1, sizeof(char));
        memcpy(node->function_name, identifier_token->string, length);

        if(!consume(")")) {
          do {
            da_append(&node->arguments, assign());
          } while(consume(","));

          expect(')');
        }
        return node;
      }

      LocalVariable *local_var = find_lvar(identifier_token);

      if (!local_var)
        error_at(identifier_token->string, "Undeclared variable");

      return new_node_local_variable(local_var);
    }

    return new_node_number(expect_number());
}