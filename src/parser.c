#include "type.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <parser.h>
#include <sys/types.h>
#include <token.h>
#include <da.h>
#include <error.h>

typedef struct Scope Scope;
typedef struct StructTag StructTag;

struct Scope {
  Scope *parent;
  LocalVariable *locals;
};

struct StructTag {
  Token *name;
  Type *type;
  StructTag *next;
};

static StructTag *struct_tags;

static StructTag *find_struct_tag(Token *name) {
  for(StructTag *tag = struct_tags; tag; tag = tag->next) {
    if (tag->name->length == name->length &&
        memcmp(tag->name->string, name->string, name->length) == 0)
        return tag;
  }

  return NULL;
}

static Scope *current_scope;
static int local_offset;
static int max_local_offset;

static Program *current_program;

static void enter_scope() {
  Scope *scope = calloc(1,sizeof(*scope));
  scope->parent = current_scope;
  current_scope = scope;
}

Node *new_node(NodeKind kind, Node *left_hand_side, Node *right_hand_side) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->left_hand_side = left_hand_side;
  node->right_hand_side = right_hand_side;
  return node;
}

Node *new_node_number(int value) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NODE_NUMBER;
  node->value = value;
  node->type = type_int;
  return node;
}

bool consume_kind(TokenKind kind) {
  if (global_token->kind != kind)
    return false;
  advance_tokenizer();
  return true;
}

bool is_type_start() {
  if(global_token->kind == TOKEN_INT ||
    global_token->kind == TOKEN_U8 ||
    global_token->kind == TOKEN_CHAR ||
    global_token->kind == TOKEN_STRUCT)
    return true;

  return false;
}

Type *parse_type();

static Type *parse_struct_type() {
  Token *name = consume_identifier();
  if(!name)
    error_at(global_token->string, "Expected a struct name");

  StructTag *tag = find_struct_tag(name);
  if(!consume("{")) {
    if(!tag)
      error_at(name->string, "Unkown struct type");
    return tag->type;
  }

  if (current_scope)
    error_at(name->string, "Struct definitions require top-levle scope");

  if (tag)
    error_at(name->string, "Struct with this name already defined.");

  Type *type = calloc(1, sizeof(*type));
  type->kind = TYPE_STRUCT;
  type->alignment = 1;

  tag = calloc(1, sizeof(*tag));
  tag->name = name;
  tag->type = type;
  tag->next = struct_tags;
  struct_tags = tag;

  StructMember **tail = &type->members;
  int offset = 0;

  while (!consume("}")) {
    if(at_eof())
      error("Expected '} before end of struct");

    Type *member_type = parse_type();
    Token *member_name = consume_identifier();

    if(!member_name)
      error_at(global_token->string, "Expected a member name");

    expect(';');

    for(StructMember *m = type->members; m; m = m->next) {
      if(m->length == member_name->length &&
        memcmp(m->name, member_name->string, member_name->length) == 0)
        error_at(member_name->string, "Duplicate member name");
    }

    if(member_type->size == 0)
      error_at(member_name->string, "Member requires a complete type");

    StructMember *member = calloc(1, sizeof(*member));
    member->name = calloc(member_name->length + 1, 1);
    memcpy(member->name, member_name->string, member_name->length);
    member->length = member_name->length;
    member->type = member_type;

    int alignment = member_type->alignment;
    offset = (offset + alignment - 1) / alignment * alignment;

    member->offset = offset;
    offset += member_type->size;
    
    if(alignment > type->alignment)
      type->alignment = alignment;

    *tail = member;
    tail = &member->next;
  }

  if(!type->members)
    error_at(name->string, "Declareing a struct without members is not supported");

  type->size = (offset + type->alignment - 1) / type->alignment * type->alignment;
  return type;
}

Type *parse_type() {
  Type *type;

  if (consume_kind(TOKEN_INT))
    type = type_int;
  else if (consume_kind(TOKEN_U8))
    type = type_u8;
  else if (consume_kind(TOKEN_CHAR))
    type = type_u8;
  else if(consume_kind(TOKEN_STRUCT))
    type = parse_struct_type();
  else
    error_at(global_token->string, "Expected a type");

  while (consume("*"))
    type = pointer_to(type);

  return type;
}

static LocalVariable *find_lvar_in_scope(Token *token) {
  for(LocalVariable *var = current_scope->locals; var; var = var->next) {
    if(var->length == token->length && !memcmp(token->string, var->name, var->length))
      return var;
  }
  return NULL;
}

LocalVariable *find_lvar(Token* token) {
  for(Scope *scope = current_scope; scope; scope = scope->parent)
    for (LocalVariable *var = scope->locals; var; var = var->next) 
      if(var->length == token->length && !memcmp(token->string, var->name, var->length))
        return var;
  return NULL;
}

static GlobalVariable *find_global(Token *token) {
  for(size_t i = 0; i <current_program->globals.count; i++) {
    GlobalVariable *global = current_program->globals.items[i];

    if(global->length == token->length && !memcmp(global->name, token->string, token->length)) {
      return global;
    }
  }
  return NULL;
}

LocalVariable *new_local_variable(Token *token, Type *type) {
    LocalVariable *local_var = calloc(1, sizeof(LocalVariable));

    local_var->name = token->string;
    local_var->length = token->length;
    local_var->type = type;

    int end = local_offset + type->size;

    local_var->offset = (end + type->alignment - 1) / type->alignment * type->alignment;

    local_offset = local_var->offset;
    if(local_offset > max_local_offset)
      max_local_offset = local_offset;

    local_var->next = current_scope->locals;
    current_scope->locals = local_var;

    return local_var;
}

Node *new_node_local_variable(LocalVariable *local_var) {
    Node *node = calloc(1, sizeof(*node));
    node->kind = NODE_LOCAL_VARIABLE;
    node->offset = local_var->offset;
    node->type = local_var->type;
    return node;
}

static Node *parse_block_contents() {
  Node *node = new_node(NODE_BLOCK, NULL, NULL);
  expect('{');

  while (!consume("}")) {
    if (at_eof())
      error("Expected '}' before end of input");
    da_append(&node->body, statement());
  }

  return node;
}

Node *parse_block() {

  Scope *parent = current_scope;
  enter_scope();

  Node *node = parse_block_contents();
  
  current_scope = parent;
  return node;
}

static void parse_paramaters(Function *function) {
  Type *parameter_type = parse_type();

  Token *param_name = consume_identifier();
  if (!param_name)
    error_at(global_token->string, "Expected a paramater name");
  if (find_lvar_in_scope(param_name))
    error_at(global_token->string, "Duplicate paramater name");
  if (function->paramaters.count >= 6)
    error_at(param_name->string, "At most 6 paramaters allowed");

  LocalVariable *argument = new_local_variable(param_name, parameter_type);
  da_append(&function->paramaters, argument);
}

void parse_function_bodies(Program *program) {
  current_program = program;

  for (size_t i = 0; i < program->functions.count; i++) {
    Function *function = program->functions.items[i];

    set_user_input(function->source_text);
    set_token(function->body_tokens);

    current_scope = NULL;
    enter_scope();

    current_scope->locals = function->locals;

    local_offset = function->locals ? function->locals->offset : 0;
    max_local_offset = local_offset;

    function->body = parse_block_contents();
    function->locals = current_scope->locals;
    function->stack_size = (max_local_offset + 15) / 16 * 16;

    current_scope = NULL;
  }
}

static Function *find_function(Token *name) {
  for (size_t i = 0; i < current_program->functions.count; i++) {
    Function *function = current_program->functions.items[i];

    if (strlen(function->name) == name->length &&
        memcmp(function->name, name->string, name->length) == 0)
        return function;
  }
  
  return NULL;
}

static void skip_function_body() {
  expect('{' );
  int depth = 1;

  while (depth > 0) {
    if (at_eof())
      error("Expected '} before end of function");

    if (consume("{"))
      depth++;
    else if (consume("}"))
      depth--;
    else
     advance_tokenizer();
  }
}

static Function *parse_function(Type *type, Token *name) {
  if (!name)
    error_at(global_token->string, "Expected a function name");

  Function *function = calloc(1, sizeof(Function));
  function->return_type = type;

  function->name = calloc(name->length + 1, sizeof(char));
  memcpy(function->name, name->string, name->length);

  current_scope = NULL;
  local_offset = 0;
  max_local_offset =0 ;
  enter_scope();

  expect('(');

  if(!consume(")")) {
    do {
      parse_paramaters(function);
    } while(consume(","));
    expect(')');
  }

  function->body_tokens = global_token;
  function->source_text = get_user_input();
  function->locals = current_scope->locals;

  skip_function_body();

  current_scope = NULL;
  return function;
}

static void parse_global_variable(Program *program, Type *type, Token *name) {
  GlobalVariable *global = calloc(1, sizeof(*global));

  global->name = calloc(name->length + 1, 1);
  memcpy(global->name, name->string, name->length);
  global->length = name->length;
  global->type = type;

  da_append(&program->globals, global);
  expect(';');
}

void parse_source(Program *program, const char *source_path) {
  current_program = program;

  while (!at_eof()) {
    if(consume_kind(TOKEN_IMPORT)) {
      Token *path = global_token;

      if(path->kind != TOKEN_STRING_LITERAL)
        error_at(path->string, "Expected a string literal");

      if(path->string_length == 0 || memchr(path->string_value, '\0', path->string_length))
        error_at(path->string, "Invalid import path");

      ImportRequest request = {
        .path = path->string_value,
        .from_path = source_path,
      };

      da_append(&program->imports, request);
      advance_tokenizer();
      continue;
    }
    
    Type *type = parse_type();
    if (type->kind == TYPE_STRUCT && consume(";"))
      continue;

    Token *name = consume_identifier();
    if(!name)
      error_at(global_token->string, "Expected a global or function name");

    if (find_global(name) || find_function(name))
      error_at(name->string, "Name already declared");
    
    if(global_token->kind == TOKEN_RESERVED && global_token->string[0] == '(')
      da_append(&program->functions, parse_function(type,name));
    else
     parse_global_variable(program, type, name);
  }
}

Node *parse_if() {
  Node *node = new_node(NODE_IF, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = statement();

  if(consume_kind(TOKEN_ELSE))
      node->els = statement(); 
  return node;
}

Node *parse_while() {
  Node *node = new_node(NODE_WHILE, NULL, NULL);
  expect('(');
  node->condition = expression();
  expect(')');
  node->then = statement();

  return node;
}

Node *parse_for() {
    Node *node = new_node(NODE_FOR, NULL, NULL);
    expect('(');
    if (!consume(";")) {
        node->init = expression();
        expect(';');
    }
    if (!consume(";")) {
        node->condition = expression();
        expect(';');
    }
    if (!consume(")")) {
        node->inc = expression();
        expect(')');
    }
    node->then = statement();
    return node;
}

Node *parse_decleration(Type *type) { 
  Token *name = consume_identifier();
  
  if(!name)
    error_at(global_token->string, "Expected a variable name");

  if(find_lvar_in_scope(name))
    error_at(global_token->string, "Variable already declared");

  LocalVariable *variable = new_local_variable(name, type);

  Node *block = new_node(NODE_BLOCK, NULL, NULL);
  
  if(consume("=")) {
    Node *left = new_node_local_variable(variable);
    Node *right = assign();

    Node *init = new_node(NODE_ASSIGN, left, right);
    da_append(&block->body, init);
  }

  expect(';');
  return block;
}

Node *statement() {
  Node *node;

  if(is_type_start())
    return parse_decleration(parse_type());
  if (consume_kind(TOKEN_IF))
    return parse_if();
  if (consume_kind(TOKEN_WHILE))
    return parse_while();
  if (consume_kind(TOKEN_FOR))
    return parse_for();
  if (consume_kind(TOKEN_BREAK)) {
    expect(';');
    return new_node(NODE_BREAK, NULL, NULL);
  }
  if (consume_kind(TOKEN_CONTINUE)) {
    expect(';');
    return new_node(NODE_CONTINUE, NULL, NULL);
  }

  if (global_token->kind == TOKEN_RESERVED && global_token->string[0] == '{')
    return parse_block();

  if (consume_kind(TOKEN_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = NODE_RETURN;
    node->left_hand_side = expression();
  } else {
    node = expression();
  }

  if (!consume(";"))
    error_at(global_token->string, "Expected a ;");

  return node;
}

Node *expression() { return assign(); }

Node *assign() {
  Node *node = equality();
  if(consume("=")) {
    node = new_node(NODE_ASSIGN, node, assign());
    node->type = node->left_hand_side->type;
  }
  return node;
}

Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume("=="))
      node = new_node(NODE_EQUALS, node, relational());
    else if (consume("!="))
      node = new_node(NODE_NOT_EQUALS, node, relational());
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();
  for (;;) {
    if (consume(">"))
      node = new_node(NODE_LESS_THAN, add(), node);
    else if (consume("<"))
      node = new_node(NODE_LESS_THAN, node, add());
    else if (consume(">="))
      node = new_node(NODE_EQUAL_OR_LESS_THAN, add(), node);
    else if (consume("<="))
      node = new_node(NODE_EQUAL_OR_LESS_THAN, node, add());
    else
      return node;
  }
}

Node *new_additive_node(NodeKind kind, Node *left, Node *right) {
  TypeKind left_kind = left->type->kind;
  TypeKind right_kind = right->type->kind;
  bool left_integer = left_kind == TYPE_INT || left_kind == TYPE_U8;
  bool right_integer = right_kind == TYPE_INT || right_kind == TYPE_U8;
  bool left_pointer = left_kind == TYPE_POINTER;
  bool right_pointer = right_kind == TYPE_POINTER;

  if (left_integer && right_integer) {
    Node *node = new_node(kind, left, right);
    node->type = type_int;
    return node;
  }

  if(left_pointer && right_integer) {
    Node *scaled = new_node(NODE_MULTIPLY, right, new_node_number(left->type->base->size));
    scaled->type = type_int;

    Node *node = new_node(kind, left, scaled);
    node->type = left->type;
    return node;
  }

  if (left_integer && right_pointer) {
    if (kind == NODE_SUBTRACT)
      error("Cannot subtract a pointer from an integer");

    return new_additive_node(kind, right, left);
  }

  if(left_pointer && right_pointer) {
    if(kind == NODE_ADD)
        error("Can not add two pointers together");

    if (!types_equal(left->type->base, right->type->base))
      error("Cannot subtract pointers to different types");
    Node *diff = new_node(NODE_SUBTRACT, left, right);
    diff->type = type_int;

    Node *node = new_node(NODE_DIVIDE, diff, new_node_number(left->type->base->size));
    node->type = type_int;
    return node;
  }

  error("Invalid operands for additive expression");
  return NULL;
}

Node *add() {
  Node *node = multiply();
  for (;;) {
    if (consume("+"))
      node = new_additive_node(NODE_ADD, node, multiply());
    else if(consume("-"))
      node = new_additive_node(NODE_SUBTRACT, node, multiply());
    else
      return node;
  }
}

Node *multiply() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(NODE_MULTIPLY, node, unary());
      node->type = type_int;
    }
    else if (consume("/")) {
      node = new_node(NODE_DIVIDE, node, unary());
      node->type = type_int;
    }
    else
      return node;
  }
}

Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_node(NODE_SUBTRACT, new_node_number(0), unary());
  if (consume("++"))
    return new_node(NODE_PRE_INCREMENT, unary(), NULL);
  if (consume("--"))
    return new_node(NODE_PRE_DECREMENT, unary(), NULL);
  if (consume("*")) {
    Node *operand = unary();

    if (!operand->type || operand->type->kind != TYPE_POINTER)
      error("Cannot dereference a non-pointer");

    Node *node = new_node(NODE_DEREFERENCE, operand, NULL);
    node->type = operand->type->base;
    return node;
  }

  if (consume("&")) {
    Node *operand = unary();

  if (operand->kind != NODE_LOCAL_VARIABLE &&
      operand->kind != NODE_GLOBAL_VARIABLE &&
      operand->kind != NODE_DEREFERENCE &&
      operand->kind != NODE_MEMBER)
      error("Cannot take address of a non-lvalue");

    Node *node = new_node(NODE_ADDRESS, operand, NULL);
    node->type = pointer_to(operand->type);
    return node;
  }
  return postfix();
}

Node *parse_index(Node *node) {
  Node *index = expression();
  expect(']');

  if(node->type->kind != TYPE_POINTER)
    error("Attempted to index a non pointer");  

  Node *add = new_additive_node(NODE_ADD, node, index);
  Node *deref = new_node(NODE_DEREFERENCE, add, NULL);
  deref->type = add->type->base;
  return deref;
}

static Node *parse_member(Node* object) {
  if (object->type->kind != TYPE_STRUCT)
    error("Member access requires a struct");

  Token *name = consume_identifier();
  if(!name)
    error_at(global_token->string, "Expected a member name.");
  
  for(StructMember *member = object->type->members; member; member = member->next) { 
    if(member->length == name->length && 
      memcmp(member->name, name->string, member->length) == 0) {
      Node *node = new_node(NODE_MEMBER, object, NULL);
      node->member = member;
      node->type = member->type;
      return node;
    }
  }

  error_at(name->string, "Unkown member of struct");
  return NULL;
}

Node *postfix() {
    Node *node = primary();
    for(;;) {  
      if(consume("++"))
        node = new_node(NODE_POST_INCREMENT, node, NULL);
      else if(consume("--"))
        node = new_node(NODE_POST_DECREMENT, node, NULL);
      else if(consume("[")) 
        node = parse_index(node);
      else if(consume("."))
        node = parse_member(node);
      else if(consume("->")) {
        if(node->type->kind != TYPE_POINTER || node->type->base->kind != TYPE_STRUCT)
          error_at(global_token->string, "Arrow access requires a pointer to a struct");

        Node *deref = new_node(NODE_DEREFERENCE, node, NULL);
        deref->type = node->type->base;

        node = parse_member(deref);
      }
      else
        return node;
    }
}

Node *primary() {
    if (consume("(")) {
        Node *node = expression();
        expect(')');
        return node;
    }

    if(global_token->kind == TOKEN_CHAR_LITERAL) {
        int value = global_token->value;
        advance_tokenizer();

        Node* node = new_node_number(value);
        node->type = type_int;
        return node;
    }

    if(global_token->kind == TOKEN_STRING_LITERAL) {
      Node *node = new_node(NODE_STRING_LITERAL, NULL, NULL);
      node->type = pointer_to(type_u8);
      node->string_length = global_token->string_length;
      node->string_value = calloc(node->string_length + 1, 1);
      memcpy(node->string_value, global_token->string_value, node->string_length);

      advance_tokenizer();
      return node;
    }

    Token *identifier_token = consume_identifier();
    if (identifier_token) {
      // its a function  
      if(consume("(")) {
        bool is_syscall = identifier_token->length == 7 && memcmp(identifier_token->string, "syscall", 7) == 0;

        Function *function = find_function(identifier_token);

        if(!function && !is_syscall)
          error_at(identifier_token->string, "Undeclared function");

        Node *node = new_node(NODE_FUNCTION_CALL, NULL, NULL);
        node->token = identifier_token;
        node->type = is_syscall ? type_int : function->return_type;

        size_t length = identifier_token->length;
        node->function_name = calloc(length +1, sizeof(char));
        memcpy(node->function_name, identifier_token->string, length);

        if(!consume(")")) {
          do {
            da_append(&node->arguments, assign());
          } while(consume(","));

          expect(')');
        }
        if (!is_syscall && node->arguments.count != function->paramaters.count)
          error_at(identifier_token->string,
              "Expected %zu arguments, got %zu",
              function->paramaters.count,
              node->arguments.count);
        return node;
      }

    // variable handling
    LocalVariable *local_var = find_lvar(identifier_token);
    if (local_var)
      return new_node_local_variable(local_var);

    GlobalVariable *global = find_global(identifier_token);
    if (global) {
      Node *node = calloc(1, sizeof(*node));
      node->kind = NODE_GLOBAL_VARIABLE;
      node->global = global;
      node->type = global->type;
      return node;
    }

    error_at(identifier_token->string, "Undeclared variable");
    }

    return new_node_number(expect_number());
}