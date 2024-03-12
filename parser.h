#ifndef __PARSER_H__
#define __PARSER_H__

#include "common.h"
#include "lexer.h"

typedef struct Parser Parser;
typedef struct Assoc Assoc;

typedef struct Type Type;
typedef struct Kind Kind;
typedef struct Expr Expr;
typedef struct Sort Sort;
typedef struct Annot Annot;

typedef struct Type {
  union {
    char term[BUF_LEN];
    struct { Expr* lhs; Expr* rhs; } arrow;
  };
} Type;

typedef struct Kind {
  union {
    EMPTY term;
    struct { Kind* lhs; Kind* rhs; } arrow;
  };
} Kind;

typedef enum AnnotType {
  ANN_EMPTY,
  ANN_TYPED,
} AnnotType;

typedef struct Annot {
  AnnotType typ;
  union {
    EMPTY empty;
    Expr* ann;
  };
} Annot;

typedef enum ExprType {
  EXP_FREE,
  EXP_TERM,
  EXP_KIND,
  EXP_APP,
  EXP_LAM,
  EXP_PI,
} ExprType;

typedef struct Expr {
  int row;
  int col;

  ExprType typ;
  Annot annot;

  union {
    struct { char name[BUF_LEN]; int idx; } term;
    char free[BUF_LEN];
    struct { Expr* lhs; Expr* rhs; } app;
    struct { Expr* lhs; Expr* rhs; } lam;
    struct { Expr* lhs; Expr* rhs; int dep; } pi;
  };
} Expr;

int parse (VEC(Token) tokens, Expr* res);

int is_infix (TokenType typ);
int is_prefix (TokenType typ);

int is_expr_sort (ExprType typ);
int is_expr_func (ExprType typ);

int parse_app   (Parser* parser, Assoc assoc, Expr* lhs, Expr* res);
int parse_arrow (Parser* parser, Assoc assoc, Expr* lhs, Expr* res);
int parse_annot (Parser* parser, Assoc assoc, Expr* lhs);
int parse_lam   (Parser* parser, Assoc assoc, Expr* res);
int parse_pi    (Parser* parser, Assoc assoc, Expr* res);

int parse_expr (Parser* parser, Assoc assoc, Expr* res);
int parse_expr_infix (Parser* parser, Assoc assoc, Expr* lhs, Expr* res);
int parse_expr_prefix (Parser* parser, Expr* res);
Assoc expr_assoc (TokenType typ);

void print_type (Type* type);
void print_expr (Expr* expr);

#endif
