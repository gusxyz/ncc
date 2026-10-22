#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <parser.h>
#include <token.h>
#include <da.h>
#include <error.h>

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
  return node;
}

bool consume_kind(TokenKind kind) {
  if (global_token->kind != kind)
    return false;
  advance_tokenizer();
  return true;
}

Program *program() {
  Program *program = calloc(1, sizeof(Program));

  while (!at_eof()) {
    da_append(&program->statements, statement());
  }

  return program;
}

Node *statement() {
  Node *node;

  if(consume_kind(TOKEN_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = NODE_RETURN;
    node->left_hand_side = expression();
  } else {
    node = expression();
  }

  if(!consume(";"))
    error_at(global_token->string, "Expected a ;");
  return node;
}

Node *expression() { return assign(); }

Node *assign() {
  Node *node = equality();
  if(consume("=")) 
    node = new_node(NODE_ASSIGN, node, assign());
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

Node *add() {
  Node *node = multiply();
  for (;;) {
    if (consume("+"))
      node = new_node(NODE_ADD, node, multiply());
    else if (consume("-"))
      node = new_node(NODE_SUBTRACT, node, multiply());
    else
      return node;
  }
}

Node *multiply() {
  Node *node = unary();
  for (;;) {
    if (consume("*"))
      node = new_node(NODE_MULTIPLY, node, unary());
    else if (consume("/"))
      node = new_node(NODE_DIVIDE, node, unary());
    else
      return node;
  }
}

Node *unary() {
  if (consume("+"))
    return unary();
  else if (consume("-"))
    return new_node(NODE_SUBTRACT, new_node_number(0), unary());
  return primary();
}

LocalVariable *locals;

LocalVariable *find_lvar(Token* token) {
  for (LocalVariable *var = locals; var; var = var->next) 
    if(var->length == token->length && !memcmp(token->string, var->name, var->length))
      return var;
  return NULL;
}

LocalVariable *new_local_variable(Token *token) {
    LocalVariable *local_var = calloc(1, sizeof(LocalVariable));

    local_var->name = token->string;
    local_var->length = token->length;
    local_var->offset = locals ? locals->offset + 8 : 8;

    local_var->next = locals;
    locals = local_var;

    return local_var;
}

Node *primary() {
    if (consume("(")) {
        Node *node = expression();
        expect(')');
        return node;
    }

    Token *identifier_token = consume_identifier();

    if (identifier_token) {
        LocalVariable *local_var = find_lvar(identifier_token);

        if (!local_var)
            local_var = new_local_variable(identifier_token);

        Node *node = calloc(1, sizeof(Node));
        node->kind = NODE_LOCAL_VARIABLE;
        node->offset = local_var->offset;

        return node;
    }

    return new_node_number(expect_number());
}

