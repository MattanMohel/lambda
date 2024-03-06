#include "validate.h"
#include "common.h"
#include "hashmap.h"
#include "parser.h"
#include "strop.h"

typedef struct Pair {
  Expr* expr;
  Type* type;
} Pair;

typedef struct Context {
  int index;
  VEC(Pair) pairs; 
} Context;

Expr term (const char* tex) {
  VEC(Token) toks = tokenize(tex);
  Expr res;
  parse(toks, &res);
  return res;
}

int validate (Expr* expr) {
  Context ctx;
  ctx.index = 0;
  ctx.pairs = VEC_NEW(Pair, 2);

  int pass = type_check(&ctx, expr);

  for (int i = 0; i < VEC_LEN(ctx.pairs); i++) {
    print_expr(ctx.pairs[i].expr);
    printf(": ");
    print_type(ctx.pairs[i].type);
    printf("\n");
  }

  return pass;
}

void check () {
  Expr a = term("\\x: a->(a->a)->b y:a->a z:a. x (y z) y");
  printf("passed: %d\n", validate(&a));  
}

int expr_eq (Expr* lhs, Expr* rhs) {
  if (rhs->vrt != lhs->vrt) return 0;
  switch (lhs->vrt) {
    case EXP_TERM: return strcmp(lhs->term, rhs->term) == 0;
    case EXP_ABST: return expr_eq(lhs->abst.lhs, rhs->abst.lhs) && expr_eq(lhs->abst.rhs, rhs->abst.rhs);
    case EXP_APPL: return expr_eq(lhs->appl.lhs, rhs->appl.lhs) && expr_eq(lhs->appl.rhs, rhs->appl.rhs);
  }
}

int type_eq (Type* lhs, Type* rhs) {
  if (lhs->vrt != rhs->vrt) return 0;
  switch (lhs->vrt) {
    case TYP_TERM: return strcmp(lhs->term, rhs->term) == 0;
    case TYP_COMP: return type_eq(lhs->comp.lhs, rhs->comp.lhs) && type_eq(lhs->comp.rhs, rhs->comp.rhs);
  }
}

int is_subtype (Type* type, Type* sub) {
  if (type_eq(type, sub)) return 1;
  switch (type->vrt) {
    case TYP_COMP: return is_subtype(type->comp.lhs, sub) || is_subtype(type->comp.rhs, sub);
    case TYP_TERM: return 0;
  }
}

void type_subst (Type* term, Type* type, Type* subst) {
  if (type_eq(term, type)) *term = *subst;
  else if (term->vrt == TYP_COMP) {
    type_subst(term->comp.lhs, type, subst);
    type_subst(term->comp.rhs, type, subst);
  }
}

void ctx_subst (Context* ctx, Type* type, Type* subst) {
  Type cpy = *type;
  type_subst(type, &cpy, subst);
  for (int i = 0; i < VEC_LEN(ctx->pairs); i++) {
    type_subst(ctx->pairs[i].type, &cpy, subst);
  }
}

//  unification:
//  ------------------------------------------------------
// | T0 = T1               | T0 <=> T1                    |
// | T0 = T1->T2->...->Tn  | T0 <= T1->T2->...->Tn        |
// | T0->T1 = T2->T3       | unify(T0, T2), unify(T1, T3) |
// | Ta = T0               | Ta <=> T0, T0 is determinate |
//  ------------------------------------------------------
// 
// errors:
//  ----------------------------------------
// | Ta = T0->T1                            |
// | T0 = T1, T0 != T1, T0 is subterm of T1 |
//  ----------------------------------------
//
//  NOTE: 'Ta' represents an explicit determinate type

int unify (Context* ctx, Type* lhs, Type* rhs) {
  printf("unify ");
  print_type(lhs);
  printf(" ");
  print_type(rhs);
  printf("\n");

  if (type_eq(lhs, rhs)) return 1;
  if (lhs->vrt != rhs->vrt) {
    if (lhs->det || rhs->det || is_subtype(lhs, rhs) || is_subtype(rhs, lhs)) return 0;
    if (lhs->vrt == TYP_COMP) ctx_subst(ctx, rhs, lhs);
    else ctx_subst(ctx, lhs, rhs);
    return 1;
  }

  switch (lhs->vrt) {
    case TYP_COMP: 
      return 
        unify(ctx, lhs->comp.lhs, rhs->comp.lhs) && 
        unify(ctx, lhs->comp.rhs, rhs->comp.rhs);

    case TYP_TERM: {
      if (lhs->det && rhs->det) return 0;
      if (lhs->det) ctx_subst(ctx, rhs, lhs); 
      else ctx_subst(ctx, lhs, rhs);          
      return 1;
    }
  }
}

int push_type(Context *ctx, Expr *expr, Type *type) {
  for (int i = 0; i < VEC_LEN(ctx->pairs); i++) {
    if (expr_eq(ctx->pairs[i].expr, expr)) return unify(ctx, ctx->pairs[i].type, type);
  }

  Pair pair; pair.expr = expr; pair.type = type;
  VEC_PUSH(ctx->pairs, pair); 
  return 1;
}

Type* new_type (Context* ctx) {
  Type* type = malloc(sizeof(Type));
  type->vrt = TYP_TERM;
  type->det = 0;  
  format_to("t%d", type->term, BUF_LEN, ctx->index);
  ctx->index++;
  return type;
}

Type* arrow (Type* lhs, Type* rhs) {
  Type* type = malloc(sizeof(Type));
  type->vrt = TYP_COMP;
  type->comp.lhs = lhs;
  type->comp.rhs = rhs;
  return type;
}

int type_check (Context *ctx, Expr *expr) {
  switch (expr->vrt) {
    case EXP_TERM: {
      if (expr->type != NULL) return push_type(ctx, expr, expr->type);
      else return push_type(ctx, expr, new_type(ctx));
    }
    case EXP_ABST: {
      Type* t1 = new_type(ctx);
      Type* t2 = new_type(ctx);

      return 
        push_type(ctx, expr, arrow(t1, t2)) &&
        push_type(ctx, expr->abst.lhs, t1)  && 
        push_type(ctx, expr->abst.rhs, t2)  && 
        type_check(ctx, expr->abst.lhs)     && 
        type_check(ctx, expr->abst.rhs);
    }
    case EXP_APPL: {
      Type* t1 = new_type(ctx);
      Type* t2 = new_type(ctx);

      return 
        push_type(ctx, expr, t2)                      && 
        push_type(ctx, expr->appl.lhs, arrow(t1, t2)) && 
        push_type(ctx, expr->appl.rhs, t1)            && 
        type_check(ctx, expr->appl.lhs)               && 
        type_check(ctx, expr->appl.rhs);
    }
  }
}


