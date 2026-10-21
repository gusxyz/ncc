#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "token.h"
#include "error.h"

Token *global_token;

void advance_tokenizer() {
  global_token = global_token->next_token;
}

// if next token is the expected symbol advance and return true
// if not return false
bool consume(char *op) {
  if(global_token->kind != TOKEN_RESERVED 
      || strlen(op) != global_token->length
      || memcmp(global_token->string, op, global_token->length))
    return false;
  advance_tokenizer();
  return true;
}


// error if next token isnt expected token
void expect(char op) {
  if(global_token->kind != TOKEN_RESERVED || global_token->string[0] != op)
    error("Expected '%c'", op);
  advance_tokenizer();
}

int expect_number() {
  if(global_token->kind != TOKEN_NUM)
    error("Expected a number, got token kind %d, character '%c'",global_token->kind, global_token->string[0]);
  int val = global_token->value;
  advance_tokenizer();
  return val;
}

bool at_eof() {
  return global_token->kind == TOKEN_EOF;
}

// init a new token and link it as next
Token *token_new(TokenKind kind, Token *current_token, char* string) {
  Token *token = calloc(1, sizeof(Token));
  token->kind = kind;
  token->string = string;
  token->length = 1;
  current_token->next_token = token;
  return token;
}

Token *tokenize(char *current_char){
  Token head_token;
  head_token.next_token = NULL;
  Token *current_token = &head_token;
  
  while(*current_char) {
    if(isspace(*current_char)) {
      current_char++;
      continue;
    }

    if(strncmp(current_char, "==", 2) == 0 ||
      strncmp(current_char, ">=", 2) == 0 ||
      strncmp(current_char, "<=", 2) == 0 ||
      strncmp(current_char, "!=", 2) == 0) {
      current_token = token_new(TOKEN_RESERVED, current_token, current_char);
      current_token->length = 2;
      current_char += 2;
    }

  if(*current_char == '+' || *current_char == '-' ||
  *current_char == '*' || *current_char == '/' ||
  *current_char == '(' || *current_char == ')' ||
  *current_char == '<' || *current_char == '>') {
      current_token = token_new(TOKEN_RESERVED, current_token, current_char++);
      continue;
    }
    
    if(isdigit(*current_char)) {
      current_token = token_new(TOKEN_NUM, current_token, current_char);
      current_token->value = strtol(current_char, &current_char, 10);
      continue;
    }

    error("Cannot tokenize input.");
  }

  token_new(TOKEN_EOF, current_token, current_char);
  return head_token.next_token;
}

void set_token(Token* tok) {
  global_token = tok;
}

Token *get_token() {
  return global_token;
}
