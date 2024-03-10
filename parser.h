#ifndef __PARSER_H__
#define __PARSER_H__

#include "common.h"
#include "lexer.h"

typedef struct Parser Parser;
typedef struct Assoc Assoc;
typedef struct Type Type;
typedef struct Expr Expr;

// (∀a.λx:a.x) [int] 10

typedef enum {
  TYP_TERM,
  TYP_COMP,
  TYP_POLY,
} TypeType;

typedef struct Type {
  TypeType vrt;
  int det;
  union {
    char term[BUF_LEN];
    struct { Type* lhs; Type* rhs; } comp;
    struct { Type* lhs; Type* rhs; } poly;
  };

  int row;
  int col;
} Type;

typedef enum {
  EXP_TERM,
  EXP_ABST,
  EXP_APPL,
  EXP_ABST_TYPE,
  EXP_APPL_TYPE,
} ExprType;

typedef struct Expr {
  ExprType vrt;
  Type* type;
  union {
    char term[BUF_LEN];
    struct { Expr* lhs; Expr* rhs; } abst;
    struct { Expr* lhs; Expr* rhs; } appl;
    struct { Type* lhs; Expr* rhs; } abst_type;
    struct { Expr* lhs; Type* rhs; } appl_type;
  };

  int row;
  int col;
} Expr;


int parse (VEC(Token) tokens, Expr* res);

int parse_type (Parser* parser, Assoc assoc, Type* res);
int parse_type_infix (Parser* parser, Assoc assoc, Type* lhs, Type* res);
int parse_type_prefix (Parser* parser, Type* res);
Assoc type_assoc (TokenType typ);

int parse_expr (Parser* parser, Assoc assoc, Expr* res);
int parse_expr_infix (Parser* parser, Assoc assoc, Expr* lhs, Expr* res);
int parse_expr_prefix (Parser* parser, Expr* res);
Assoc expr_assoc (TokenType typ);

void print_type (Type* type);
void print_expr (Expr* expr);

#endif
