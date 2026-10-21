#include "parser.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

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

Node *expression() { return equality(); }

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

Node *primary() {
  if (consume("(")) {
    Node *node = expression();
    expect(')');
    return node;
  }

  return new_node_number(expect_number());
}

void generate_code(Node *node) {
  // Push Integers to Stack so we can actually multiply
  if (node->kind == NODE_NUMBER) {
    printf("  push %d\n", node->value);
    return;
  }

  generate_code(node->left_hand_side);
  generate_code(node->right_hand_side);


  // Pop last two Ints from the stack so we can do math.
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case NODE_ADD:
    printf("  add rax, rdi\n");
    break;
  case NODE_SUBTRACT:
    printf("  sub rax, rdi\n");
    break;
  case NODE_EQUALS:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzx rax, al\n");
    break;
  case NODE_NOT_EQUALS:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzx rax, al\n");
    break;
  case NODE_LESS_THAN:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzx rax, al\n");
    break;
  case NODE_EQUAL_OR_LESS_THAN:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzx rax, al\n");
    break;
  case NODE_MULTIPLY:
    printf("  imul rax, rdi\n");
    break;
  case NODE_DIVIDE:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}
