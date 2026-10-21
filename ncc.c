#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include "token.h"

void print_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

Token *token;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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

// Tokenize an input


int main(int argc, char**argv) {
	if(argc != 2) {
    fprintf(stderr, "Incorrect number of args\n");	
    return 1;
	}

  token = tokenize(argv[1]);

  print_header();
  printf("  mov rax, %d\n", expect_number());
  
  while(!at_eof()) {
    if(consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    };
    expect('-');
    printf("  sub rax, %d\n", expect_number());
  };
  printf("  ret\n");
  return 0;
}
