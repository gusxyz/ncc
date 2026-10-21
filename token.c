#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "token.h"
#include "error.h"

Token *token;

void advance() {
  token = token->next;
}

// if next token is the expected symbol advance and return true
// if not return false
bool consume(char op) {
  if(token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  advance();
  return true;
}


// error if next token isnt expected token
void expect(char op) {
  if(token->kind != TK_RESERVED || token->str[0] != op)
    error("Expected '%c', op");
  advance();
}

int expect_number() {
  if(token->kind != TK_NUM)
    error("Expected a number");
  int val = token->val;
  advance();
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// init a new token and link it as next
Token *token_new(TokenKind kind, Token *cur, char* str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

Token *tokenize(char *p){
  Token head;
  head.next = NULL;
  Token *cur = &head;
  
  while(*p) {
    if(isspace(*p)) {
      p++;
      continue;
    }

    if(*p == '+' || *p == '-') {
      cur = token_new(TK_RESERVED, cur, p++);
      continue;
    }
    
    if(isdigit(*p)) {
      cur = token_new(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error("Cannot tokenize input.");
  }

  token_new(TK_EOF, cur, p);
  return head.next;
}

void set_token(Token* tok) {
  token = tok;
}
