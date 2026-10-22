#include <stdio.h>
#include "codegen.h"
#include "error.h"

void generate_local_variable(Node* node) {
  if(node->kind != NODE_LOCAL_VARIABLE)
    error("Left-hand side of assignemnt is not a variable");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void generate_assign(Node *node) {
    generate_local_variable(node->left_hand_side);
    generate_code(node->right_hand_side);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
}

void generate_code(Node *node) {
  // NUMBERS, VARIABLES AND ASSIGNMENTS FIRST.
  if(node->kind == NODE_RETURN) {
    generate_code(node->left_hand_side);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  switch(node->kind) {
  case NODE_NUMBER:
    printf("  push %d\n", node->value);
    return;
  case NODE_LOCAL_VARIABLE:
    generate_local_variable(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case NODE_ASSIGN:
    generate_assign(node);
    return;
  default:
    break;
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
