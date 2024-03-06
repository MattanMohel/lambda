#include <stdio.h>
#include "vec.h"
#include "lexer.h"
#include "parser.h"
#include "validate.h"

int main () {
  Expr r1 = term("\\x.x y z");
  Expr r2 = term("\\x.x y y");

  printf("eq: %b\n", expr_eq(&r1, &r2));
  check();
  return 0;
}
