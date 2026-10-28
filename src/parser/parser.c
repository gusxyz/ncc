#include "token/token.h"
#include <parser/parser_internal.h>

#include <da.h>
#include <error/error.h>
#include <stdlib.h>
#include <string.h>
#include <import/import.h>

Program *current_program;
Function *current_function;

bool consume_kind(TokenKind kind) {
  if (global_token->kind != kind)
    return false;
  advance_tokenizer();
  return true;
}

GlobalVariable *find_global(Token *token) {
  for (size_t i = 0; i < current_program->globals.count; i++) {
    GlobalVariable *global = current_program->globals.items[i];

    if (global->length == token->length &&
        !memcmp(global->name, token->string, token->length)) {
      return global;
    }
  }
  return NULL;
}

Function *find_function(Token *name) {
  for (size_t i = 0; i < current_program->functions.count; i++) {
    Function *function = current_program->functions.items[i];
    if (strlen(function->name) == name->length &&
        memcmp(function->name, name->string, name->length) == 0)
      return function;
  }
  return NULL;
}

static void parse_paramaters(Function *function) {
  Token *param_name = consume_identifier();
  if (!param_name)
    error_at(global_token->string, "Expected a paramater name");
  expect(':');
  Type *parameter_type = parse_type();

  if (find_lvar_in_scope(param_name))
    error_at(global_token->string, "Duplicate paramater name");
  if (function->paramaters.count >= 6)
    error_at(param_name->string, "At most 6 paramaters allowed");

  LocalVariable *argument = new_local_variable(param_name, parameter_type);
  da_append(&function->paramaters, argument);
}

static void skip_function_body() {
  expect('{');
  int depth = 1;

  while (depth > 0) {
    if (at_eof())
      error_at(global_token->string,"Expected '}' before end of function");

    if (consume("{"))
      depth++;
    else if (consume("}"))
      depth--;
    else
      advance_tokenizer();
  }
}

static Function *parse_function(Type *type, Token *name, char* source_path) {
  if (!name)
    error_at(global_token->string, "Expected a function name");

  Function *function = calloc(1, sizeof(Function));
  function->return_type = type;
  function->source_path = source_path;

  function->name = calloc(name->length + 1, sizeof(char));
  memcpy(function->name, name->string, name->length);

  current_scope = &global_scope;
  local_offset = 0;
  max_local_offset = 0;
  enter_scope();

  expect('(');

  if (!consume(")")) {
    do {
      parse_paramaters(function);
    } while (consume(","));
    expect(')');
  }

  function->body_tokens = global_token;
  function->source_text = get_user_input();
  function->locals = current_scope->locals;

  skip_function_body();

  current_scope = &global_scope;
  return function;
}

static void parse_global_variable(Program *program, Type *type, Token *name) {
  if (type->kind == TYPE_VOID)
    error_at(name->string, "Global variable cannot have type void");

  GlobalVariable *global = calloc(1, sizeof(*global));

  global->name = calloc(name->length + 1, 1);
  memcpy(global->name, name->string, name->length);
  global->length = name->length;
  global->type = type;

  da_append(&program->globals, global);
  expect(';');
}


bool name_exists(Token *token) {
  if(find_global(token) ||
    find_function(token) ||
    find_constant_in_scope(token) ||
    find_type_alias_in_current_scope(token))
    return true;
  return false;
}

void parse_source(Program *program, char *source_path) {
  current_program = program;
  current_scope = &global_scope;

  while (!at_eof()) {
    if (consume_kind(TOKEN_IMPORT)) {
      Token *path = global_token;

      if (path->kind != TOKEN_STRING_LITERAL)
        error_at(path->string, "Expected a string literal");

      if (path->string_length == 0 ||
          memchr(path->string_value, '\0', path->string_length))
        error_at(path->string, "Invalid import path");

      ImportRequest request = {
          .path = path->string_value,
          .from_path = source_path,
      };

      da_append(&program->imports, request);
      advance_tokenizer();
      continue;
    }

    Token *next = global_token->next_token;

    if (global_token->kind == TOKEN_IDENTIFIER &&
        next &&
        next->kind == TOKEN_RESERVED &&
        next->length == 2 &&
        memcmp(next->string, "::",2) == 0) {
          
      if (name_exists(global_token))
        error_at(global_token->string, "Name already declared");

      parse_decleration();
      continue;
    }

    if (global_token->kind == TOKEN_IDENTIFIER &&
        next &&
        next->kind == TOKEN_RESERVED &&
        next->length == 1 &&
        next->string[0] == ':') {
      Token *name = consume_identifier();
      expect(':');
      Type *type = parse_type();
      if (name_exists(name))
        error_at(name->string, "Name already declared");

      parse_global_variable(program, type, name);
      continue;
    }

    Type *type = parse_type();
    if (type->kind == TYPE_STRUCT && consume(";"))
      continue;

    Token *name = consume_identifier();
    if (!name)
      error_at(global_token->string, "Expected a global or function name");

    if (name_exists(name))
      error_at(name->string, "Name already declared");

    if (global_token->kind == TOKEN_RESERVED && global_token->string[0] == '(') 
      da_append(&program->functions, parse_function(type, name, source_path));
    else 
      parse_global_variable(program, type, name);
  }
}

void parse_function_bodies(Program *program) {
  current_program = program;

  for (size_t i = 0; i < program->functions.count; i++) {
    Function *function = program->functions.items[i];
    set_user_input(function->source_text);
    set_source_path(function->source_path);
    set_token(function->body_tokens);

    current_scope = &global_scope;
    enter_scope();

    current_scope->locals = function->locals;

    local_offset = function->locals ? function->locals->offset : 0;
    max_local_offset = local_offset;

    current_function = function;
    function->body = parse_block_contents();
    current_function = NULL;
    function->locals = current_scope->locals;
    function->stack_size = (max_local_offset + 15) / 16 * 16;

    current_scope = &global_scope;
  }
}

static void parse_source_tree(Program *prog, char *source, char* source_path) {
  Token *tokens = tokenize(source);
  int depth = 0;

  // Discover imports before parsing any declerations.
  for (Token *token = tokens; token && token->kind != TOKEN_EOF; token = token->next_token) {
    // handle depth
    if(token->kind == TOKEN_RESERVED && token->length == 1) {
      if(token->string[0] == '{')
        depth++;
      else if(token->string[0] == '}')
        depth--;
    }

    if (depth != 0 || token->kind != TOKEN_IMPORT)
      continue;
    
    set_user_input(source);
    set_source_path(source_path);

    Token *path_token = token->next_token;

    if(!path_token || path_token->kind != TOKEN_STRING_LITERAL)
      error_at(token->string, "Expected an import path string literal.");

    if(path_token->string_length == 0 ||
      memchr(path_token->string_value, '\0',path_token->string_length))
      error_at(path_token->string, "Invalid import path");

    char *path = resolve_import_path(source_path, path_token->string_value);

    if(!mark_source_loaded(path)) {
      free(path);
      continue;
    }
    char *imported_source = read_source_file(path);

    parse_source_tree(prog, imported_source, path);
  }

  set_user_input(source);
  set_source_path(source_path);
  set_token(tokens);
  parse_source(prog, source_path);
}

Program *parse(char *source, char *source_path) {
  Program *prog = calloc(1, sizeof(*prog));

  if(source_path)
    mark_source_loaded(source_path);

  parse_source_tree(prog, source, source_path);
  parse_function_bodies(prog);

  bool has_main = false;

  for (size_t i = 0; i < prog->functions.count; i++) 
    if (strcmp(prog->functions.items[i]->name, "main") == 0)
      has_main = true;

  if (!has_main)
    error("No defined main function");

  return prog;
}