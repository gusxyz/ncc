#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <token.h>

char *user_input;

void set_user_input(char *new_user_input) { user_input = new_user_input; }
char *get_user_input() { return user_input; }

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // Print pos spaces
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void expect(char op) {
  if(global_token->kind != TOKEN_RESERVED || global_token->string[0] != op)
    error_at(global_token->string, "Expected '%c'", op);
  advance_tokenizer();
}

int expect_number() {
  if(global_token->kind != TOKEN_NUMBER)
    error("Expected a number, got token kind %d, character '%c'",global_token->kind, global_token->string[0]);
  int val = global_token->value;
  advance_tokenizer();
  return val;
}