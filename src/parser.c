#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <parser.h>
#include <token.h>
#include <da.h>
#include <error.h>

LocalVariable *locals;

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

Function *parse_function() {
  if (!consume_kind(TOKEN_INT))
    error_at(global_token->string, "Expected 'int'");

  Token *name = consume_identifier();
  if (!name)
    error_at(global_token->string, "Expected a function name");

  Function *function = calloc(1, sizeof(Function));

  function->name = calloc(name->length + 1, sizeof(char));
  memcpy(function->name, name->string, name->length);

  locals = NULL;  expect('(');

  if(!consume(")")) {
    do {
      if(!consume_kind(TOKEN_INT))
        error_at(global_token->string, "Expected argument type 'int'");

      Token *param_name = consume_identifier();
      if(!param_name)
        error_at(global_token->string, "Expected an argument name");
      if(find_lvar(param_name))
        error_at(global_token->string, "Duplicate argument name");
      if(function->arguments.count >= 6)
        error_at(param_name->string, "At most 6 paramaters allowed");

      LocalVariable *argument = new_local_variable(param_name);
      da_append(&function->arguments, argument);
    } while(consume(","));
    expect(')');
  }

  function->body = parse_block();

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

Node *parse_block() {
  Node *node = new_node(NODE_BLOCK, NULL, NULL);
  expect('{');

  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of input");
    da_append(&node->body, statement());
  }

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

Node *statement() {
  Node *node;

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
  if (consume("-"))
    return new_node(NODE_SUBTRACT, new_node_number(0), unary());
  if (consume("++"))
    return new_node(NODE_PRE_INCREMENT, unary(), NULL);
  if (consume("--"))
    return new_node(NODE_PRE_DECREMENT, unary(), NULL);
  return postfix();
}

Node *postfix() {
    Node *node = primary();
    for(;;) {  
      if(consume("++"))
        node = new_node(NODE_POST_INCREMENT, node, NULL);
      else if(consume("--"))
        node = new_node(NODE_POST_DECREMENT, node, NULL);
      else
        return node;
    }
}

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

Node *new_node_local_variable(LocalVariable *local_var) {
    Node *node = calloc(1, sizeof(*node));
    node->kind = NODE_LOCAL_VARIABLE;
    node->offset = local_var->offset;
    return node;
}

Node *primary() {
    if (consume("(")) {
        Node *node = expression();
        expect(')');
        return node;
    }

    Token *identifier_token = consume_identifier();

    if (identifier_token) {
      // its a function  
      if(consume("(")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = NODE_FUNCTION_CALL;

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
          local_var = new_local_variable(identifier_token);

      return new_node_local_variable(local_var);
    }

    return new_node_number(expect_number());
}

