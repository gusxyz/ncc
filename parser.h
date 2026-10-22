#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

// Structures
typedef enum {
  NODE_ADD,
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,
  NODE_EQUALS,
  NODE_NOT_EQUALS,
  NODE_GREATER_THAN,
  NODE_LESS_THAN,
  NODE_EQUAL_OR_LESS_THAN,
  NODE_EQUAL_OR_GREATER_THAN,
  NODE_NUMBER,
  NODE_ASSIGN,
  NODE_LOCAL_VARIABLE,
  NODE_RETURN,
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *left_hand_side;
  Node *right_hand_side;
  int value;
  int offset;
};

typedef struct LocalVariable LocalVariable;

struct LocalVariable {
  LocalVariable *next;
  char *name;
  int length;
  int offset;
};

typedef struct {
  Node **items;
  size_t count;
  size_t capacity;
} NodeArray;

typedef struct {
  NodeArray statements;
} Program;

// Functions
Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side);
Node *new_node_number(int value);

// Grammar
Program *program();
Node *statement();
Node *expression();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *multiply();
Node *unary();
Node *primary();

#endif
