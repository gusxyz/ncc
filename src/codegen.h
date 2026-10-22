#ifndef CODEGEN_H
#define CODEGEN_H

#include <parser.h>

void generate_statement(Node *node);
void generate_expression(Node *node);

#endif
