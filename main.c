#include <stdio.h>
#include "vec.h"
#include "lexer.h"
#include "parser.h"
#include "validate.h"

Expr test (const char* tex) {
  VEC(Token) toks = tokenize(tex);
  for (int i = 0; i < VEC_LEN(toks); i++) {
    printf("(%d) - %s\n", i, tok(toks[i].type));
  }

  Expr expr;

  if (!parse(toks, &expr)) {
    printf("failed to parse\n");
  }

  print_expr(&expr);
  printf("\n");

  return expr;
}

// !a.Lx:a->a y:a.x y : forall a . (a -> a) -> a -> a

int main (int argc, char** argv) {
  //Expr r1 = term("\\x.x y z");
  //Expr r2 = term("\\x.x y y");

  //printf("eq: %b\n", expr_eq(&r1, &r2));
  //check();
  
  if (argc != 3) {
    printf("expected an argument!\n");
    return 1;
  }

  Expr a = test(argv[1]);
  Expr b = test(argv[2]);

  int pass = expr_eq(&a, &b);

  printf("eq: %d\n", pass);

  return 0;
}
