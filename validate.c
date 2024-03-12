#include "validate.h"
#include "common.h"
#include "hashmap.h"
#include "parser.h"
#include "strop.h"

Expr term (const char* str) {
  VEC(Token) toks = tokenize(str);
  Expr res;
  parse(toks, &res);
  VEC_FREE(toks);
  return res;
}

int expr_eq (Expr *lhs, Expr *rhs) {
  if (lhs->typ != rhs->typ) return 0;

  switch (lhs->typ) {
    case EXP_KIND: return 1;
    case EXP_TERM: return lhs->term.idx == rhs->term.idx;
    case EXP_FREE: return strcmp(lhs->free, rhs->free) == 0;
    case EXP_APP: return expr_eq(lhs->app.lhs, rhs->app.lhs) && expr_eq(lhs->app.rhs, rhs->app.rhs);
    case EXP_LAM: return expr_eq(lhs->lam.lhs->annot.ann, rhs->lam.lhs->annot.ann) && expr_eq(lhs->lam.rhs, rhs->lam.rhs); 
    case EXP_PI: {
      if (lhs->pi.dep != rhs->pi.dep) return 0;
      if (!lhs->pi.dep) return expr_eq(lhs->pi.lhs, rhs->pi.lhs) && expr_eq(rhs->pi.rhs, rhs->pi.rhs);
      return expr_eq(lhs->pi.lhs->annot.ann, rhs->pi.lhs->annot.ann) && expr_eq(rhs->pi.rhs, rhs->pi.rhs);
    }
  }
}

typedef struct Pair {
  Expr* expr;
  Expr* type;
} Pair;

typedef struct Context {
  VEC(Pair) pairs; 
} Context;

Expr* ctx_push (Context* ctx, Expr* expr, Expr* type) {
  Pair pair;
  pair.expr = expr;
  pair.type = type;

  VEC_PUSH(ctx->pairs, pair); 
  return type;
}

void ctx_rem (Context* ctx, Expr* expr) {
  for (int i = VEC_LEN(ctx->pairs)-1; i >= 0; i++) {
    if (expr_eq(ctx->pairs[i].expr, expr)) {
      VEC_REM(ctx->pairs, i);
    }
  }
}

Expr* ctx_get (Context* ctx, Expr* expr) {
  if (expr->typ == EXP_KIND) return expr;
  for (int i = VEC_LEN(ctx->pairs)-1; i >= 0; i--) {
    if (expr_eq(ctx->pairs[i].expr, expr)) {
      return ctx->pairs[i].type;
    }
  }
  return NULL;
}

Expr* pi_new (Expr* lhs, Expr* rhs) {
  Expr* pi = malloc(sizeof(Expr));
  pi->typ = EXP_PI;
  pi->pi.lhs = lhs;
  pi->pi.rhs = rhs;
  pi->pi.dep = is_subterm(rhs, lhs);
  return pi;
}

Expr* subst (Expr* expr, Expr* term, Expr* sub) {
  switch (expr->typ) {
    case EXP_KIND: return expr;
    case EXP_TERM:
    case EXP_FREE: {
        if (expr->annot.typ == ANN_TYPED) expr->annot.ann = subst(expr->annot.ann, term, sub);
        if (expr_eq(expr, term)) return sub; 
        else return expr;
    }
    case EXP_APP: {
      Expr* lhs = subst(expr->app.lhs, term, sub);
      Expr* rhs = subst(expr->app.rhs, term, sub);
      Expr* app = malloc(sizeof(Expr));
      app->typ = EXP_APP;
      app->app.lhs = lhs;
      app->app.rhs = rhs;
      return app;
    }
    case EXP_LAM: {
      Expr* lam = malloc(sizeof(Expr));
      lam->typ = EXP_LAM;
      lam->lam.lhs = subst(expr->pi.lhs, term, sub);
      lam->lam.rhs = subst(expr->pi.rhs, term, sub);
      return lam;
    }
    case EXP_PI: {   
      Expr* lhs = subst(expr->pi.lhs, term, sub);
      Expr* rhs = subst(expr->pi.rhs, term, sub);
      Expr* pi = pi_new(lhs, rhs);
      return pi;
    }
  }
}

int is_subterm (Expr* expr, Expr* term) {
  if (expr->annot.typ == ANN_TYPED && is_subterm(expr->annot.ann, term)) return 1;
  switch (expr->typ) {
    case EXP_KIND: return 0;
    case EXP_TERM: 
    case EXP_FREE: return expr_eq(expr, term);
    case EXP_APP: return is_subterm(expr->app.lhs, term) || is_subterm(expr->app.rhs, term);
    case EXP_LAM: return is_subterm(expr->lam.lhs, term) || is_subterm(expr->lam.rhs, term);
    case EXP_PI: return is_subterm(expr->pi.lhs, term) || is_subterm(expr->pi.rhs, term);
  }
}

Expr* check (Expr* expr) {
  Context ctx;
  ctx.pairs = VEC_NEW(Pair, 2);
  return type_check(&ctx, expr);
}

void print_ctx (Context* ctx) {
  if (VEC_LEN(ctx->pairs) == 0) return;
  printf("---len: %d---\n", VEC_LEN(ctx->pairs));
  for (int i = 0; i < VEC_LEN(ctx->pairs); i++) {
    print_expr(ctx->pairs[i].expr);
    printf(": ");
    print_expr(ctx->pairs[i].type);
    printf("\n");
  }
  printf("------------\n");
}

Expr* type_check (Context *ctx, Expr *expr) {
  switch (expr->typ) {
    case EXP_KIND: 
    case EXP_FREE: 
    case EXP_TERM: return ctx_get(ctx, expr);
    case EXP_LAM: {
      Expr* annot = type_check(ctx, expr->lam.lhs->annot.ann);
      if (annot == NULL) return NULL;
      ctx_push(ctx, expr->lam.lhs, expr->lam.lhs->annot.ann);
      Expr* body = type_check(ctx, expr->lam.rhs);
      //ctx_rem(ctx, expr->lam.lhs);
      if (body == NULL) return NULL;
      return ctx_push(ctx, expr, pi_new(expr->lam.lhs, body)); 
    }
    case EXP_PI: {
      Expr* annot = type_check(ctx, expr->lam.lhs->annot.ann);
      if (!ctx_get(ctx, annot)) return NULL;
      ctx_push(ctx, expr->pi.lhs, expr->lam.lhs->annot.ann);
      Expr* body = type_check(ctx, expr->lam.rhs);
      if (body == NULL) return NULL;
      //ctx_rem(ctx, expr->pi.lhs);
      return ctx_push(ctx, expr, body);
    }
    case EXP_APP: {
      Expr* type = type_check(ctx, expr->app.lhs);
      if (type == NULL || type->typ != EXP_PI) return NULL;
      Expr* rhs = type_check(ctx, expr->app.rhs);
      if (!expr_eq(type->pi.lhs->annot.ann, rhs)) return NULL;

      Expr* sub = subst(type->pi.rhs, type->pi.lhs, expr->app.rhs);
      return ctx_push(ctx, expr, sub);
    }
  }
}


