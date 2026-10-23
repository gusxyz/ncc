#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  TOKEN_RESERVED,
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_EOF,
  TOKEN_RETURN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_BREAK,
  TOKEN_CONTINUE,
  TOKEN_INT,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next_token;
  int value;         
  char *string;      
  size_t length;
};

static const char *operators[] = {
    "==", "!=", "<=", ">=",
    "++", "--",
    "+", "-", "*", "/",
    "(", ")", "<", ">",
    "=", ";", "{", "}", ",",
};

extern Token *global_token;

bool consume(char *op);
Token* consume_identifier();
void expect(char op);
int expect_number();
bool at_eof();
Token *token_new(TokenKind kind, Token *current_token, char* string);
Token *tokenize(char *p);
void set_token(Token *token);
Token *get_token();
void advance_tokenizer();

#endif
