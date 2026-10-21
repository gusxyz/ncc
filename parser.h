#ifndef PARSER_H
#define PARSER_H

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
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *left_hand_side;
  Node *right_hand_side;
  int value;
};

// Functions
Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side);
Node *new_node_number(int value);
Node *expression();
Node *equality();
Node *relational();
Node *add();
Node *multiply();
Node *unary();
Node *primary();
void generate_code(Node *node);

#endif
