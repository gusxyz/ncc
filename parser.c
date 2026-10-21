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

Node *expr() {
  Node *node = mul();
  for (;;) {
    if (consume('+'))
      node = new_node(NODE_ADD, node, mul());
    else if (consume('-'))
      node = new_node(NODE_SUBTRACT, node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume('*'))
      node = new_node(NODE_MULTIPLY, node, unary());
    else if (consume('/'))
      node = new_node(NODE_DIVIDE, node, unary());
    else
      return node;
  }
}

Node *unary() {
  for (;;) {
    if (consume('+'))
      return primary();
    else if (consume('-'))
      return new_node(NODE_SUBTRACT, new_node_number(0), primary());
    return primary();
  }
}

Node *primary() {
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  return new_node_number(expect_number());
}

void generate_code(Node *node) {
  if (node->kind == NODE_NUMBER) {
    printf("  push %d\n", node->value);
    return;
  }

  generate_code(node->left_hand_side);
  generate_code(node->right_hand_side);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case NODE_ADD:
    printf("  add rax, rdi\n");
    break;
  case NODE_SUBTRACT:
    printf("  sub rax, rdi\n");
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
