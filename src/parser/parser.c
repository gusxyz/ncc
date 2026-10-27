#include <parser/parser_internal.h>

#include <da.h>
#include <error/error.h>
#include <stdlib.h>
#include <string.h>

Program *current_program;

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
  expect('{');
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

    Type *type = parse_type();
    if (type->kind == TYPE_STRUCT && consume(";"))
      continue;

    Token *name = consume_identifier();
    if (!name)
      error_at(global_token->string, "Expected a global or function name");

    if (find_global(name) || find_function(name))
      error_at(name->string, "Name already declared");

    if (global_token->kind == TOKEN_RESERVED && global_token->string[0] == '(')
      da_append(&program->functions, parse_function(type, name));
    else
      parse_global_variable(program, type, name);
  }
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
