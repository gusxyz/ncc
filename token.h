#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef enum {
  TOKEN_RESERVED, // Symbols
  TOKEN_NUM,      // Integer
  TOKEN_EOF,      // End of input
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;    // Type
  Token *next_token; // Reference to the next token
  int value;         // numeric value for TK_NUM
  char *string;      // text
};

bool consume(char op);
void expect(char op);
int expect_number();
bool at_eof();
Token *token_new(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);
void set_token(Token *token);
Token *get_token();

#endif
