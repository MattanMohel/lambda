#include "lexer.h"
#include "vec.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
  VEC(Token) tokens;
  const char* stream;
  const char* ptr;
  int row;
  int col;
  int bind_site;
} Lexer;

Token new_token (Lexer* state, TokenType type, const char* tok) {
  Token token;
  token.type = type;
  token.col = state->col;
  token.row = state->row;
  strcpy(token.tok, tok);
  return token;
}

void add_token (Lexer* state, const char* lex, TokenType type) {
  Token token = new_token(state, type, lex);
  state->col += strlen(lex);
  VEC_PUSH(state->tokens, token);
}

int take (Lexer* state, const char* pat, TokenType type) {
  if (strncmp(state->ptr, pat, strlen(pat)) == 0) {
    add_token(state, pat, type);
    state->ptr += strlen(pat);
    return 1;
  }

  return 0;
}

void take_space (Lexer* state) {
  while (isspace(*state->ptr)) {
    state->col++;
    state->ptr++; 
    if (*state->ptr == '\n') {
      state->row++;
      state->col = 0;
    }   
  }
}

int take_op (Lexer* state) {
  if (take(state, "\\", TOK_LAM) || take(state, "forall", TOK_FORALL)) {
    state->bind_site = 1;
    return 1;
  } 

  if (take(state, ".", TOK_PERIOD)) {
    state->bind_site = 0;
    return 1;
  }

  return 
    take(state, ":", TOK_COLON)    ||
    take(state, "->", TOK_ARROW)   ||
    take(state, "*", TOK_ASTERISK) ||
    take(state, "(", TOK_LPARENTH) ||
    take(state, ")", TOK_RPARENTH);
}

int take_ident (Lexer* state) {
  char lex [BUF_LEN] = {0};
  char* ptr = lex;

  while (isalpha(*state->ptr) && islower(*state->ptr)) {
    *ptr = *state->ptr;
    state->ptr++;
    ptr++;
  }

  if (ptr != lex) {
    add_token(state, lex, TOK_IDENT);
    return 1;
  }

  return 0;
}

VEC(Token) tokenize (const char* stream) {
  Lexer lex;
  lex.tokens = VEC_NEW(Token, 2);
  lex.stream = stream;
  lex.ptr = stream;
  lex.row = 0;
  lex.col = 0;
  lex.bind_site = 0;

  while (*lex.ptr != '\0') {
    do { 
      take_space(&lex); 
    } while (take_op(&lex));
    take_ident(&lex);
  }

  Token end = new_token(&lex, TOK_END_OF_INPUT, "");
  VEC_PUSH(lex.tokens, end);
  return lex.tokens;
}

const char* tok (TokenType type) {
  switch (type) {
    case TOK_LAM: return "λ";
    case TOK_APP: return "apply";
    case TOK_IDENT: return "term";
    case TOK_PERIOD: return ".";
    case TOK_ASTERISK: return "*";
    case TOK_COLON: return ":";
    case TOK_ARROW: return "->";
    case TOK_FORALL: return "∀";
    case TOK_LPARENTH: return "(";
    case TOK_RPARENTH: return ")";
    case TOK_END_OF_INPUT: return "end of input";
  }
}
