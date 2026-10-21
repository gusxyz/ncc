#ifndef TOKEN_H
#define TOKEN_H

#include<stdbool.h>

typedef enum {
  TK_RESERVED, // Symbols
  TK_NUM,  // Integer
  TK_EOF, // End of input
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // Type
  Token *next; // Reference to the next token
  int val; // numeric value for TK_NUM
  char *str; // text
};

bool consume(char op);
void expect(char op);
int expect_number();
bool at_eof();
Token *token_new(TokenKind kind, Token *cur, char* str);
Token *tokenize(char *p);
void set_token(Token *token);


#endif
