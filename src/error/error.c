#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <error/error.h>
#include <token/token.h>

char *user_input;
static const char *user_input_path = "<input>";

void set_user_input(char *new_user_input) { user_input = new_user_input; }

char *get_user_input() { return user_input; }
void set_source_path(const char *path) {
  user_input_path = path ? path : "<command-line>";
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  char *line_start = loc;
  while (line_start > user_input && line_start[-1] != '\n')
    line_start--;

  char *line_end = loc;
  while (*line_end && *line_end != '\n')
    line_end++;

  int line_number = 1;
  for (char *p = user_input; p < line_start; p++) {
    if (*p == '\n')
      line_number++;
  }

  int column = (int)(loc - line_start) + 1;
  int width = snprintf(NULL, 0, "%d", line_number);

  fprintf(stderr, "%s:%d:%d: error: ", user_input_path, line_number, column);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n%*d | %.*s\n",
          width, line_number,
          (int)(line_end - line_start), line_start);

  fprintf(stderr, "%*s | ", width, "");

  // Preserve tabs so the caret follows the displayed indentation.
  for (char *p = line_start; p < loc; p++)
    fputc(*p == '\t' ? '\t' : ' ', stderr);

  fprintf(stderr, "^\n");
  exit(1);
}

void expect(char op) {
  if(global_token->kind != TOKEN_RESERVED || global_token->string[0] != op)
    error_at(global_token->string, "Expected '%c'", op);
  advance_tokenizer();
}

int expect_number() {
  if(global_token->kind != TOKEN_NUMBER)
    error_at(global_token->string, "Expected a number, got token kind %d, character '%c'",global_token->kind, global_token->string[0]);
  int val = global_token->value;
  advance_tokenizer();
  return val;
}