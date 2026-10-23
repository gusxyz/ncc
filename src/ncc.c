#include <stdio.h>
#include <token.h>
#include <error.h>
#include <parser.h>
#include <codegen.h>
#include <emit.h>

void print_header() {
  emit_directive(".intel_syntax noprefix");
  emit_directive(".global main");
  emit_label("main");
}

void allocate_prologue() {
  // Function prologue
  // Allocate space for 26 variables
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, 208");
}

void epilogue() {
  // Function epilogue
  // Leave's last functions result in rax
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
}

int main(int argc, char**argv) {
	if(argc != 2) {
    fprintf(stderr, "Incorrect number of args\n");	
    return 1;
	}

  set_user_input(argv[1]);
  set_token(tokenize(argv[1]));

  print_header();
  allocate_prologue();
  
  Program *prog = parse_program();

  for (size_t i = 0; i < prog->statements.count; i++) {
    Node* node = prog->statements.items[i];
    generate_statement(node);

    if(node->kind != NODE_RETURN)
      printf("  pop rax\n");
  }

  epilogue();
  return 0;
}
