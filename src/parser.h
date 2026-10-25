#ifndef PARSER_H
#define PARSER_H

#include <token.h>
#include <stddef.h>
#include <type.h>

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
  NODE_GLOBAL_VARIABLE,
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
  NODE_ADDRESS,
  NODE_DEREFERENCE,
  NODE_STRING_LITERAL,
  NODE_MEMBER,
} NodeKind;

typedef struct Node Node;
typedef struct GlobalVariable GlobalVariable;
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

  char *function_name;
  NodeArray arguments;
  
  NodeArray body;

  Type* type;
  Token *token;

  int value;
  int offset;
  
  char *string_value;
  size_t string_length;
  GlobalVariable *global;
  StructMember *member;
};

typedef struct LocalVariable LocalVariable;

struct LocalVariable {
  LocalVariable *next;
  char *name;
  size_t length;
  int offset;
  Type* type;
};

struct GlobalVariable {
  char *name;
  size_t length;
  Type* type;
};
typedef struct {
  LocalVariable **items;
  size_t count;
  size_t capacity;
} ParamaterArray;

typedef struct {
  GlobalVariable **items;
  size_t count;
  size_t capacity;
} GlobalsArray;

typedef struct {
  int stack_size;
  char *source_text;
  char *name;
  Node *body;
  ParamaterArray paramaters;
  Token *body_tokens;
  LocalVariable *locals;
  Type *return_type;
} Function;

typedef struct {
  Function **items;
  size_t count;
  size_t capacity;
} FunctionArray;

typedef struct {
  char *path;
  const char* from_path;
} ImportRequest;

typedef struct {
  ImportRequest *items;
  size_t count;
  size_t capacity;
} ImportArray;

typedef struct {
  FunctionArray functions;
  GlobalsArray globals;
  ImportArray imports;
} Program;

// Functions
Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side);
Node *new_node_number(int value);
LocalVariable *find_lvar(Token *token);
LocalVariable *new_local_variable(Token *token, Type *type);

// Grammar
void parse();
void parse_source(Program *program, const char *source_path);
void parse_function_bodies(Program *program);
Node *parse_decleration(Type *type);
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
