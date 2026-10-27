#include "type/type.h"
#include <ast/ast.h>
#include <error/error.h>

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

Node *new_additive_node(NodeKind kind, Node *left, Node *right) {
  TypeKind left_kind = left->type->kind;
  TypeKind right_kind = right->type->kind;
  bool left_integer = left_kind == TYPE_INT || left_kind == TYPE_U8 || left_kind == TYPE_I64;
  bool right_integer = right_kind == TYPE_INT || right_kind == TYPE_U8 || right_kind == TYPE_I64;
  bool left_pointer = left_kind == TYPE_POINTER || left_kind == TYPE_ARRAY;
  bool right_pointer = right_kind == TYPE_POINTER || right_kind == TYPE_ARRAY;

  if (left_integer && right_integer) {
    Node *node = new_node(kind, left, right);
    node->type = type_int;
    return node;
  }

  if (left_pointer && right_integer) {
    Node *scaled =
        new_node(NODE_MULTIPLY, right, new_node_number(left->type->base->size));
    scaled->type = type_int;

    Node *node = new_node(kind, left, scaled);
    node->type = pointer_to(left->type->base);
    return node;
  }

  if (left_integer && right_pointer) {
    if (kind == NODE_SUBTRACT)
      error("Cannot subtract a pointer from an integer");

    return new_additive_node(kind, right, left);
  }

  if (left_pointer && right_pointer) {
    if (kind == NODE_ADD)
      error("Can not add two pointers together");

    if (!types_equal(left->type->base, right->type->base))
      error("Cannot subtract pointers to different types");
    Node *diff = new_node(NODE_SUBTRACT, left, right);
    diff->type = type_int;

    Node *node =
        new_node(NODE_DIVIDE, diff, new_node_number(left->type->base->size));
    node->type = type_int;
    return node;
  }

  error("Invalid operands for additive expression");
  return NULL;
}

int evaluate_constant(Node *node) {
  switch (node->kind) {
  case NODE_NUMBER:
    return node->value;
  case NODE_CAST: {
    int value = evaluate_constant(node->left_hand_side);

    if (node->type->kind == TYPE_U8)
      return (unsigned char) value;

    return value;
}  case NODE_ADD:
    return evaluate_constant(node->left_hand_side) +
           evaluate_constant(node->right_hand_side);
  case NODE_SUBTRACT:
    return evaluate_constant(node->left_hand_side) -
           evaluate_constant(node->right_hand_side);
  case NODE_MULTIPLY:
    return evaluate_constant(node->left_hand_side) *
           evaluate_constant(node->right_hand_side);
  case NODE_MODULO:
    return evaluate_constant(node->left_hand_side) %
           evaluate_constant(node->right_hand_side);
  case NODE_DIVIDE: {
    int left = evaluate_constant(node->left_hand_side);
    int right = evaluate_constant(node->right_hand_side);

    if (right == 0)
      error("Division by zero in constant expression");

    return left / right;
  }

  default:
    error("Expected a constant integer expression");
    return 0;
  }
}
