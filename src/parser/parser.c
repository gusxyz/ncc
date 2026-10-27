#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <parser/parser_internal.h>
#include <sys/types.h>
#include <da.h>
#include <error/error.h>

Program *current_program;


bool consume_kind(TokenKind kind) {
  if (global_token->kind != kind)
    return false;
  advance_tokenizer();
  return true;
}

LocalVariable *find_lvar_in_scope(Token *token) {
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

Constant *find_constant_in_scope(Token *name) {
    for(Constant *constt = current_scope->constants; constt; constt = constt->next) {
    if (constt->name->length == name->length &&
        memcmp(constt->name->string, name->string, name->length) == 0)
        return constt;
  }
  return NULL;
}

Constant *find_constant(Token* token) {
  for(Scope *scope = current_scope; scope; scope = scope->parent)
    for (Constant *constt = scope->constants; constt; constt = constt->next) 
      if(constt->name->length == token->length && !memcmp(token->string, constt->name, constt->name->length))
        return constt;
  return NULL;
}

GlobalVariable *find_global(Token *token) {
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
    }
    else if (consume("/")) {
      node = new_node(NODE_DIVIDE, node, unary());
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

Node *parse_function_call(Token *identifier_token) {
  bool is_syscall = identifier_token->length == 7 &&
                    memcmp(identifier_token->string, "syscall", 7) == 0;

  Function *function = find_function(identifier_token);

  if (!function && !is_syscall)
    error_at(identifier_token->string, "Undeclared function");

  Node *node = new_node(NODE_FUNCTION_CALL, NULL, NULL);
  node->token = identifier_token;
  node->type = is_syscall ? type_int : function->return_type;

  size_t length = identifier_token->length;
  node->function_name = calloc(length + 1, sizeof(char));
  memcpy(node->function_name, identifier_token->string, length);

  if (!consume(")")) {
    do {
      da_append(&node->arguments, assign());
    } while (consume(","));

    expect(')');
  }
  if (!is_syscall && node->arguments.count != function->paramaters.count)
    error_at(identifier_token->string,
    "Expected %zu arguments, got %zu",
             function->paramaters.count, 
             node->arguments.count);
  return node;
}

Node *primary() {
  if (consume("(")) {
    Node *node = expression();
    expect(')');
    return node;
  }

  if (global_token->kind == TOKEN_CHAR_LITERAL) {
    int value = global_token->value;
    advance_tokenizer();

    Node *node = new_node_number(value);
    node->type = type_int;
    return node;
  }

  if (global_token->kind == TOKEN_STRING_LITERAL) {
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
    if (consume("(")) {
      return parse_function_call(identifier_token);
    }

    for (Scope *scope = current_scope; scope; scope = scope->parent) {
      for (LocalVariable *var = scope->locals; var; var = var->next) {
        if (var->length == identifier_token->length &&
            memcmp(var->name, identifier_token->string, var->length) == 0)
          return new_node_local_variable(var);
      }

      for (Constant *constant = scope->constants;
          constant;
          constant = constant->next) {
        if (constant->name->length == identifier_token->length &&
            memcmp(constant->name->string, identifier_token->string, identifier_token->length) == 0) {
          Node *node = new_node_number(constant->value);
          node->type = constant->type;
          return node;
        }
      }
    }
    
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