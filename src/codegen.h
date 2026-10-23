#ifndef CODEGEN_H
#define CODEGEN_H

#include <parser.h>

void generate_statement(Node *node);
void generate_expression(Node *node);
void print_header();
void allocate_prologue(int stack_size);
void epilogue();
void generate_function(Function *function);
void generate_program(Program *program);

#endif
