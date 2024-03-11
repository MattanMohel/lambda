#ifndef __VALIDATE_H__
#define __VALIDATE_H__

#include "parser.h"

int expr_eq (Expr* lhs, Expr* rhs);
int type_eq (Type* lhs, Type* rhs);
int kind_eq (Kind* lhs, Kind* rhs);

//typedef struct Context Context;

//void check();

//Expr term (const char* tex);

//int validate (Expr* expr);
//int unify (Context* ctx, Type* lhs, Type* rhs);
//int expr_eq (Expr* lhs, Expr* rhs);
//int type_eq (Type* lhs, Type* rhs);
//int push_type (Context* ctx, Expr* expr, Type* type);
//int type_check (Context* ctx, Expr* expr);
//Type* ctx_find (Context* ctx, Expr* expr);
//void ctx_subst (Context* ctx, Type* type, Type* subst);

//Type* arrow (Type* lhs, Type* rhs);
//Type* new_type (Context* ctx);

#endif
