#include <parser/parser_internal.h>
#include <error/error.h>
#include <da.h>

Node *parse_if() {
  Node *node = new_node(NODE_IF, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = parse_statement();

  if(consume_kind(TOKEN_ELSE))
      node->els = parse_statement(); 
  return node;
}

Node *parse_while() {
  Node *node = new_node(NODE_WHILE, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = parse_statement();

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
    node->then = parse_statement();
    return node;
}

Node *parse_block_contents() {
  Node *node = new_node(NODE_BLOCK, NULL, NULL);
  expect('{');

  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of input");
    da_append(&node->body, parse_statement());
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

Node *parse_statement() {
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

  if (is_decleration())
    return parse_decleration();


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
