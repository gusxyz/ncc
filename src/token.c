#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <token.h>
#include <error.h>
#include <da.h>

Token *global_token;

void advance_tokenizer() {
  global_token = global_token->next_token;
}

static const char *operators[] = {
    "==", "!=", "<=", ">=",
    "++", "--",
    "+", "-", "*", "/", "&",
    "(", ")", 
    "[","]",
    "<", ">",
    "=", ";", 
    "{", "}", 
    ",",
};

static const Keyword keywords[] = {
  {"int",      TOKEN_INT},
  {"char",     TOKEN_CHAR},
  {"u8",       TOKEN_U8},
  {"if",       TOKEN_IF},
  {"else",     TOKEN_ELSE},
  {"while",    TOKEN_WHILE},
  {"for",      TOKEN_FOR},
  {"return",   TOKEN_RETURN},
  {"break",    TOKEN_BREAK},
  {"continue", TOKEN_CONTINUE},
};

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

Token *consume_identifier() {
  if (global_token->kind != TOKEN_IDENTIFIER)
    return NULL;

  Token *token = global_token;
  advance_tokenizer();
  return token;
}

// error if next token isnt expected token
void expect(char op) {
  if(global_token->kind != TOKEN_RESERVED || global_token->string[0] != op)
    error("Expected '%c'", op);
  advance_tokenizer();
}

int expect_number() {
  if(global_token->kind != TOKEN_NUMBER)
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

static int operator_length(const char* op) {
  for(size_t i = 0; i < sizeof(operators) / sizeof(*operators); i++) {
    int length = strlen(operators[i]);
    if (strncmp(op, operators[i], length) == 0)
      return length;
  }

  return 0;
}

// dogshit hack
int is_shmalnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         (c == '_');
}

static TokenKind keyword_kind(const char *text, size_t length) {
  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
    size_t keyword_length = strlen(keywords[i].name);

    if (length == keyword_length &&
        memcmp(text, keywords[i].name, length) == 0)
      return keywords[i].kind;
  }

  return TOKEN_IDENTIFIER;
}

static int read_literal_byte(char **cursor) {
  unsigned char value = **cursor;

  if (value == '\0' || value == '\n' || value == '\r')
    error("Unexpected end of literal");

  (*cursor)++;

  if (value != '\\')
    return value;

  // The cursor now points at the character after the backslash.
  value = **cursor;

  if (value == '\0' || value == '\n' || value == '\r')
    error("Unterminated escape sequence");

  (*cursor)++;

  switch (value) {
    case 'n':  return '\n';
    case 't':  return '\t';
    case 'r':  return '\r';
    case '0':  return '\0';
    case '\\': return '\\';
    case '"':  return '"';
    case '\'': return '\'';
    default:
      error("Unknown escape sequence: \\%c", value);
  }

  return 0;
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

    int char_length = operator_length(current_char);
    if(char_length > 0) {
      current_token = token_new(TOKEN_RESERVED, current_token, current_char);
      current_token->length = char_length;
      current_char += char_length;
      continue;
    }

    if (isalpha(*current_char) || *current_char == '_') {
      char *start = current_char;

      while (isalnum(*current_char) || *current_char == '_')
        current_char++;

      int length = current_char - start;

      current_token = token_new(keyword_kind(start, length), current_token, start);
      current_token->length = length;
      continue;
    }

    if(isdigit(*current_char)) {
      current_token = token_new(TOKEN_NUMBER, current_token, current_char);
      current_token->value = strtol(current_char, &current_char, 10);
      continue;
    }
  
    
    if (*current_char == '"') {
      char *start = current_char;
      current_char++;

      ByteArray bytes = {0};

      while (*current_char && *current_char != '"') {
        int byte = read_literal_byte(&current_char);
        da_append(&bytes, byte);
      }

      if (!*current_char)
        error("Unterminated string literal");
      
      da_append(&bytes, '\0');
      current_char++;

      Token *token = token_new(TOKEN_STRING_LITERAL, current_token, start);

      int length = bytes.count - 1;
      token->length = length;
      token->string_value = bytes.items;
      token->string_length = length;

      current_token = token;
      continue;
    }

    if (*current_char == '\'') {
      char *start = current_char + 1;
      current_char++;
      if (*current_char == '\'')
        error("Expected one character between single quotes");

      int byte = read_literal_byte(&current_char);
      if(*current_char != '\'')
        error("Expected a closing single quote");
      current_char++;

      Token *token = token_new(TOKEN_CHAR_LITERAL, current_token, start);
      token->value = byte;
      token->length = current_char - start;

      current_token = token;
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
