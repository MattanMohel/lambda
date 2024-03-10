#include <stdio.h>
#include "vec.h"
#include "lexer.h"
#include "parser.h"
#include "validate.h"

// TODO: unify forall a. a->a with b->b
// bugs with forall a . a->a = b->b->t1
// so a = b = b->t1 
// (((@a.\\x:a.x) [b]) y y

void test (const char* tex) {
  VEC(Token) toks = tokenize(tex);
  for (int i = 0; i < VEC_LEN(toks); i++) {
    printf("(%d) - %s\n", i, tok(toks[i].type));
  }

  Expr expr;

  if (!parse(toks, &expr)) {
    printf("failed to parse\n");
    return;
  }

  printf("\n");
  if (!validate(&expr)) {
    printf("failed to validate!\n");
  } 
}

// !a.Lx:a->a y:a.x y : forall a . (a -> a) -> a -> a

int main () {
  //Expr r1 = term("\\x.x y z");
  //Expr r2 = term("\\x.x y y");

  //printf("eq: %b\n", expr_eq(&r1, &r2));
  //check();
  
  test("((@a.\\x:a.x)[b]) y");

  return 0;
}
