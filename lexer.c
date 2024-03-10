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

int take (Lexer* state, const char* cmp, TokenType type) {
  if (strncmp(state->ptr, cmp, strlen(cmp)) == 0) {
    add_token(state, cmp, type);
    state->ptr += strlen(cmp);
    return 1;
  }

  return 0;
}

void take_space (Lexer* state) {
  while (isspace(*state->ptr)) {
    if (*state->ptr == '\n') {
      state->row++;
      state->col = 0;
    } else {
      state->col++;
    }
    state->ptr++; 
  }
}

int take_op (Lexer* state) {
  if (take(state, "\\", TOK_ABST) || take(state, "@", TOK_FORALL)) {
    state->bind_site = 1;
    return 1;
  } if (take(state, ".", TOK_BODY)) {
    state->bind_site = 0;
    return 1;
  }

  return 
    take(state, ":", TOK_TYPE)     || 
    take(state, "->", TOK_ARROW)   ||
    take(state, "*", TOK_ASTERISK) ||
    take(state, "(", TOK_LPARENTH) ||
    take(state, ")", TOK_RPARENTH) ||
    take(state, "[", TOK_LBRACKET) ||
    take(state, "]", TOK_RBRACKET);
}

int take_ident (Lexer* state) {
  char  lex [BUF_LEN] = { '\0' };
  char* ptr = lex;

  while (isalpha(*state->ptr)) {
    *ptr = *state->ptr;
    state->ptr++;
    ptr++;
  }

  if (ptr != lex) {
    add_token(state, lex, TOK_TERM);
    return 1;
  }

  return 0;
}

// if `ident || ')'` && `(` || ')' && `ident || '('`
int take_infix (Lexer* state) {
  take_space(state);

  if (!take_ident(state) && !take(state, ")", TOK_RPARENTH) && !take(state, "]", TOK_RBRACKET)) return 0;

  while (1) {
    Token token = new_token(state, TOK_APPL, "");
    VEC_PUSH(state->tokens, token);
    take_space(state);
    if (take(state, "(", TOK_LPARENTH) || take(state, "[", TOK_LBRACKET)) {
      return 1;
    } else if (!take_ident(state)) {
      VEC_POP(state->tokens);
      return 1;
    } 
  }
}

VEC(Token) tokenize (const char* stream) {
  Lexer state;
  state.tokens = VEC_NEW(Token, 2);
  state.stream = stream;
  state.ptr = stream;
  state.row = 0;
  state.col = 0;
  state.bind_site = 0;

  while (*state.ptr != '\0') {
    if (state.bind_site) {
      take_space(&state);
      take_ident(&state);
      take_space(&state);
      take_op(&state);
    } else {
      if (!take_infix(&state)) {
        take_op(&state);
      }
    }
  }

  Token end = new_token(&state, TOK_END_OF_INPUT, "\0");
  VEC_PUSH(state.tokens, end);
  return state.tokens;
}

int is_prefix (TokenType type) {
  return type == TOK_ABST; 
}

int is_infix (TokenType type) {
  return type == TOK_APPL || type == TOK_TYPE || type == TOK_ARROW;
}

const char* tok (TokenType type) {
  switch (type) {
    case TOK_APPL: return "application";
    case TOK_ABST: return "\\";
    case TOK_TERM: return "term";
    case TOK_BODY: return ".";
    case TOK_TYPE: return ":";
    case TOK_ARROW: return "->";
    case TOK_FORALL: return "forall";
    case TOK_LPARENTH: return "(";
    case TOK_RPARENTH: return ")";
    case TOK_LBRACKET: return "[";
    case TOK_RBRACKET: return "]";
    case TOK_END_OF_INPUT: return "end of input";
    default: return "[no token]";
  }
}
