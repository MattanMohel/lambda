#ifndef __LEXER_H__
#define __LEXER_H__

#include "common.h"

typedef enum {
  TOK_IDENT,
  TOK_APP,
  TOK_LAM,
  TOK_FORALL,
  TOK_PERIOD,
  TOK_COLON,
  TOK_ARROW,
  TOK_ASTERISK,
  TOK_LPARENTH,
  TOK_RPARENTH,
  TOK_END_OF_INPUT,
} TokenType;

typedef struct {
  TokenType type;
  char tok[BUF_LEN];
  int  row;
  int  col;
} Token;

typedef struct {
  int beg_row;
  int beg_col;
  int end_row;
  int end_col;
} Span;

VEC(Token) tokenize (const char* stream);

const char* tok (TokenType type);

#endif
