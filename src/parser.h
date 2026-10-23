#ifndef PARSER_H
#define PARSER_H

#include <token.h>
#include <stddef.h>

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
  NODE_IF,
  NODE_WHILE,
  NODE_FOR,
  NODE_BLOCK,
  NODE_EXPR_STMT,
  NODE_PRE_INCREMENT,
  NODE_POST_INCREMENT,
  NODE_PRE_DECREMENT,
  NODE_POST_DECREMENT,
  NODE_BREAK,
  NODE_CONTINUE,
  NODE_FUNCTION_CALL,
} NodeKind;

typedef struct Node Node;

typedef struct {
  Node **items;
  size_t count;
  size_t capacity;
} NodeArray;

struct Node {
  NodeKind kind;

  Node *left_hand_side;
  Node *right_hand_side;

  Node *condition;
  Node *then;
  Node *els;

  Node *init;
  Node *inc;

  NodeArray body;
  NodeArray arguments;

  int value;
  int offset;
  char *function_name;
};

typedef struct LocalVariable LocalVariable;

struct LocalVariable {
  LocalVariable *next;
  char *name;
  size_t length;
  int offset;
};

typedef struct {
  LocalVariable **items;
  size_t count;
  size_t capacity;
} ArgumentArray;

typedef struct {
  char *name;
  Node *body;
  ArgumentArray arguments;
  int stack_size;
} Function;

typedef struct {
  Function **items;
  size_t count;
  size_t capacity;
} FunctionArray;

typedef struct {
  FunctionArray functions;
} Program;

// Functions
Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side);
Node *new_node_number(int value);
LocalVariable *find_lvar(Token *token);
LocalVariable *new_local_variable(Token *token);

// Grammar
Program *parse_program();
Node *parse_block();
Node *parse_decleration();
Node *statement();
Node *expression();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *multiply();
Node *unary();
Node *postfix();
Node *primary();

#endif
