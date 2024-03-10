#ifndef __LEXER_H__
#define __LEXER_H__

#include "common.h"

typedef enum {
  TOK_TERM,
  TOK_ABST,
  TOK_APPL,
  TOK_BODY,
  TOK_TYPE,
  TOK_ARROW,
  TOK_FORALL,
  TOK_ASTERISK,
  TOK_LPARENTH,
  TOK_RPARENTH,
  TOK_LBRACKET,
  TOK_RBRACKET,
  TOK_END_OF_INPUT,
} TokenType;

 // (∏Σα.λx:α.x)β
 // (\\a:*.\\x:a.x)b
 // (forall a . \\x:a.x)b

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
int is_prefix (TokenType type);
int is_infix  (TokenType type);

const char* tok (TokenType type);

#endif
