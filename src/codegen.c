#include <stdio.h>
#include <codegen.h>
#include <error.h>
#include <emit.h>

void generate_local_variable(Node* node) {
  if(node->kind != NODE_LOCAL_VARIABLE)
    error("Left-hand side of assignemnt is not a variable");
  emit("mov rax, rbp");
  emit("sub rax, %d", node->offset);
  emit("push rax");
}

void generate_assign(Node *node) {
    generate_local_variable(node->left_hand_side);
    generate_code(node->right_hand_side);

    emit("pop rdi");
    emit("pop rax");
    emit("mov [rax], rdi");
    emit("push rdi");
}


static int label_counter;

void generate_if(Node *node) {
  // implement
}

void generate_code(Node *node) {
  // NUMBERS, VARIABLES AND ASSIGNMENTS FIRST.
  if(node->kind == NODE_RETURN) {
    generate_code(node->left_hand_side);
    emit("pop rax");
    emit("mov rsp, rbp");
    emit("pop rbp");
    emit("ret");
    return;
  }

  switch(node->kind) {
  case NODE_NUMBER:
    emit("push %d", node->value);
    return;
  case NODE_LOCAL_VARIABLE:
    generate_local_variable(node);
    emit("pop rax");
    emit("mov rax, [rax]");
    emit("push rax");
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
  emit("pop rdi");
  emit("pop rax");

  switch (node->kind) {
  case NODE_ADD:
    emit("add rax, rdi");
    break;
  case NODE_SUBTRACT:
    emit("sub rax, rdi");
    break;
  case NODE_EQUALS:
    emit("cmp rax, rdi");
    emit("sete al");
    emit("movzx rax, al");
    break;
  case NODE_NOT_EQUALS:
    emit("cmp rax, rdi");
    emit("setne al");
    emit("movzx rax, al");
    break;
  case NODE_LESS_THAN:
    emit("cmp rax, rdi");
    emit("setl al");
    emit("movzx rax, al");
    break;
  case NODE_EQUAL_OR_LESS_THAN:
    emit("cmp rax, rdi");
    emit("setle al");
    emit("movzx rax, al");
    break;
  case NODE_MULTIPLY:
    emit("imul rax, rdi");
    break;
  case NODE_DIVIDE:
    emit("cqo");
    emit("idiv rdi");
    break;
  }

  emit("push rax");
}
