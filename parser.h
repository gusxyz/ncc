#ifndef PARSER_H
#define PARSER_H

// Structures
typedef enum {
  NODE_ADD, 
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,
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
Node *new_node(NodeKind kind, Node* left_hand_side, Node* right_hand_side);
Node *new_node_number(int value);
Node *expr();
Node *mul();
Node *primary();
void generate_code(Node *node);

#endif
