#ifndef __VALIDATE_H__
#define __VALIDATE_H__

#include "parser.h"

typedef struct Context Context;

typedef enum Level {
  LEV_TYPE,
  LEV_KIND,
} Level;

typedef struct Sort {
  Level level;
  union {
    Type type;
    Kind kind;
  };
} Sort;

void print_ctx(Context* ctx);
Expr term (const char* tex);

int expr_eq (Expr* lhs, Expr* rhs);

int is_subterm (Expr* expr, Expr* term);
Expr* subst (Expr* expr, Expr* term, Expr* sub);

Expr* check (Expr* expr);
Expr* type_check (Context* ctx, Expr* expr);



//int validate (Expr* expr);
//int unify (Context* ctx, Type* lhs, Type* rhs);
//int push_type (Context* ctx, Expr* expr, Type* type);
//int type_check (Context* ctx, Expr* expr);
//Type* ctx_find (Context* ctx, Expr* expr);
//void ctx_subst (Context* ctx, Type* type, Type* subst);

//Type* arrow (Type* lhs, Type* rhs);
//Type* new_type (Context* ctx);

#endif
