#include <ast/ast.h>

Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->left_hand_side = left_hand_side;
  node->right_hand_side = right_hand_side;

    switch (kind) {
    case NODE_EQUALS:
    case NODE_NOT_EQUALS:
    case NODE_LESS_THAN:
    case NODE_EQUAL_OR_LESS_THAN:
    case NODE_SUBTRACT:
    case NODE_MULTIPLY:
    case NODE_DIVIDE:
      node->type = type_int;
      break;
    case NODE_PRE_INCREMENT:
    case NODE_PRE_DECREMENT:
    case NODE_POST_INCREMENT:
    case NODE_POST_DECREMENT:
      node->type = left_hand_side->type;
      break;
    default:
      break;
  }

  return node;
}

Node *new_node_number(int value) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NODE_NUMBER;
  node->value = value;
  node->type = type_int;
  return node;
}

Node *new_node_local_variable(LocalVariable *local_var) {
    Node *node = calloc(1, sizeof(*node));
    node->kind = NODE_LOCAL_VARIABLE;
    node->offset = local_var->offset;
    node->type = local_var->type;
    return node;
}