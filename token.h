#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>


typedef enum {
  TOKEN_RESERVED,
  TOKEN_NUM,  
  TOKEN_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next_token;
  int value;         
  char *string;      
  size_t length;
};

bool consume(char *op);
void expect(char op);
int expect_number();
bool at_eof();
Token *token_new(TokenKind kind, Token *current_token, char* string);
Token *tokenize(char *p);
void set_token(Token *token);
Token *get_token();

#endif
