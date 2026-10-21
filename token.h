#ifndef TOKEN_H
#define TOKEN_H

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

#endif
