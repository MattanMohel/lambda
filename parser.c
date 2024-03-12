#include "parser.h"
#include "strop.h"
#include "hashmap.h"
#include <stdarg.h>

typedef struct Parser {
  VEC(Token) stream;
  int binding;
  int dummies;
  Hashmap* binds;
  Token* ptr; 
  char mes [BUF_LEN];
} Parser;

typedef enum {
  RASSOC = 0,
  LASSOC = 1,
} AssocType;

typedef struct Assoc {
  AssocType type;
  int bp;
} Assoc;

int push_err (Parser* parser, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  format_arg_to(fmt, parser->mes, BUF_LEN, args);
  return 0;
}

Token* at (Parser* parser) {
  return parser->ptr;
}

int eof (Parser* parser) {
  return at(parser)->type == TOK_END_OF_INPUT;
}

int expect (Parser* parser, TokenType typ) {
  if (at(parser)->type == typ) return 1;
  push_err(parser, "expected '%s', found '%s'", tok(typ), tok(at(parser)->type));
  return 0;
}

Token* eat (Parser* parser) {
  Token* tok = at(parser);
  if (!eof(parser)) parser->ptr++;
  return tok;
}

int try_eat (Parser* parser, TokenType typ) {
  if (!expect(parser, typ)) return 0;
  eat(parser);
  return 1;
}

Assoc new_assoc (AssocType typ, int bp) {
  Assoc assoc;
  assoc.type = typ;
  assoc.bp = bp;
  return assoc;
}

Expr new_expr () {
  Expr expr;
  expr.annot.typ = ANN_EMPTY;
  return expr;
}

int assoc_bp (Assoc assoc) {
  if (assoc.type == RASSOC) {
    return assoc.bp;
  } else {
    return assoc.bp + 1;
  }
}

Expr* expr_alloc (Expr* expr) {
  Expr* alloc = malloc(sizeof(Expr));
  memcpy(alloc, expr, sizeof(Expr));
  return alloc;
}

Type* type_alloc (Type* type) {
  Type* alloc = malloc(sizeof(Type));
  memcpy(alloc, type, sizeof(Type));
  return alloc;
}

int is_infix (TokenType typ) {
  return 
    typ == TOK_COLON || 
    typ == TOK_APP || 
    typ == TOK_ARROW;
}

int is_prefix (TokenType typ) {
  return 
    typ == TOK_FORALL || 
    typ == TOK_LAM;
}

int is_expr_term (ExprType typ) {
  return 
    typ == EXP_TERM || 
    typ == EXP_FREE;
}

int is_expr_sort (ExprType typ) {
  return 
    typ == EXP_TERM || 
    typ == EXP_FREE || 
    typ == EXP_KIND ||
    typ == EXP_PI;
}

int is_expr_func (ExprType typ) {
  return 
    typ == EXP_LAM || 
    typ == EXP_PI;
}

char* expr_ident (Expr* expr) {
  switch (expr->typ) {
    case EXP_FREE: return expr->free;
    case EXP_TERM: return expr->term.name;
    default: return "";
  }
}

void push_bind (Parser* parser, Expr* expr) {
  int* index = malloc(sizeof(int));
  *index = parser->binds->len;
  hmap_add(parser->binds, expr_ident(expr), index);

  // transform free variable to a binding one
  expr->typ = EXP_TERM;
  expr->term.idx = parser->binds->len - 1;
}

void rem_bind (Parser* parser, Expr* expr) {
  hmap_rem(parser->binds, expr_ident(expr));
}

Assoc expr_assoc (TokenType type) {
  switch (type) {
    case TOK_APP:    return new_assoc(LASSOC, 50);
    case TOK_ARROW:  return new_assoc(RASSOC, 70);
    case TOK_LAM:    return new_assoc(LASSOC, 30);
    case TOK_COLON:  return new_assoc(LASSOC, 60);
    case TOK_FORALL: return new_assoc(LASSOC, 30);
    default:         return new_assoc(RASSOC, -1);
  }
}

int parse (VEC(Token) tokens, Expr* res) {
  Parser parser;
  memset(parser.mes, '\0', BUF_LEN);
  parser.stream = tokens;
  parser.ptr = tokens;
  parser.binding = 0;
  parser.dummies = 0;
  parser.binds = hmap_new();

  int pass = parse_expr(&parser, new_assoc(RASSOC, 0), res);

  if (!pass) {
    printf("%s at (%d, %d)\n", parser.mes, parser.ptr->col, parser.ptr->row);
  }

  //hmap_delete(parser.binds);
  return pass;
}

int parse_expr (Parser* parser, Assoc assoc, Expr* res) {
  Expr lhs = new_expr(); 

  switch (at(parser)->type) {
    case TOK_IDENT: {
      Token* cur = eat(parser);  
      int* idx = hmap_get(parser->binds, cur->tok);

      if (idx == NULL) {
        lhs.typ = EXP_FREE;
        strcpy(lhs.free, cur->tok);
      } else {
        lhs.typ = EXP_TERM;
        lhs.term.idx = *idx;
        strcpy(lhs.term.name, cur->tok);
      }

      lhs.col = cur->col;
      lhs.row = cur->row;
      break;
    }
    case TOK_ASTERISK: {
      Token* cur = eat(parser);
      lhs.typ = EXP_KIND;
      lhs.col = cur->col;
      lhs.row = cur->row;
      break;
    }
    case TOK_LPARENTH: {
      eat(parser);
      if (!parse_expr(parser, new_assoc(RASSOC, 0), &lhs)) return 0;
      if (!try_eat(parser, TOK_RPARENTH)) return 0;
      break;
    }
    default: {
      if (!parse_expr_prefix(parser, &lhs)) return 0;
      break;
    }
  }

  Expr* alloc = expr_alloc(&lhs);
  if (!parse_expr_infix(parser, assoc, alloc, res)) {
    free(alloc);
    return 0;
  } 
  return 1;
}

int parse_expr_infix (Parser* parser, Assoc assoc, Expr* lhs, Expr* res) {
  TokenType op = at(parser)->type; 
  
  // if lhs is adjacent to 'ident' or '(' then apply
  if (!parser->binding && (op == TOK_IDENT || op == TOK_LPARENTH)) {
    op = TOK_APP;
  } 

  if (!is_infix(op) || expr_assoc(op).bp < assoc_bp(assoc)) {
    memcpy(res, lhs, sizeof(Expr));
    free(lhs);
    return 1;
  }

  if (op != TOK_APP) eat(parser);
  
  switch (op) {    
    case TOK_COLON: 
      if (!parse_annot(parser, expr_assoc(op), lhs)) return 0;
      return parse_expr_infix(parser, assoc, lhs, res);
    
    case TOK_ARROW: {
      Expr infix = new_expr();
      if (!parse_arrow(parser, expr_assoc(op), lhs, &infix)) return 0;
      Expr* alloc = expr_alloc(&infix);
      if (!parse_expr_infix(parser, assoc, alloc, res)) {
        free(alloc);
        return 0;
      } 
      return 1;
    }
    
    case TOK_APP: {
      Expr infix = new_expr(); 
      if (!parse_app(parser, expr_assoc(op), lhs, &infix)) return 0;
      
      Expr* alloc = expr_alloc(&infix);
      if (!parse_expr_infix(parser, assoc, alloc, res)) {
        free(alloc);
        return 0;
      } else {
        return 1;
      }
    }
    default: 
      return push_err(parser, "expected infix, found '%s'", tok(op));
  }
}

int parse_expr_prefix (Parser* parser, Expr* res) {
  TokenType op = eat(parser)->type; 
  switch (op) {
    case TOK_LAM: return parse_lam(parser, expr_assoc(TOK_LAM), res);
    case TOK_FORALL: return parse_pi(parser, expr_assoc(TOK_FORALL), res);
    default: return push_err(parser, "expected prefix, found '%s'", tok(op));
  }
}

int parse_app (Parser* parser, Assoc assoc, Expr* lhs, Expr* res) {
  Expr rhs = new_expr(); 
  if (!parse_expr(parser, assoc, &rhs)) return 0;

  res->typ = EXP_APP;
  res->app.lhs = lhs;
  res->app.rhs = expr_alloc(&rhs);
  return 1;
}

int parse_arrow (Parser* parser, Assoc assoc, Expr* lhs, Expr* res) {  
  if (!is_expr_sort(lhs->typ)) return push_err(parser, "cannot form arrow of non-sort lhs term");  
  Expr rhs = new_expr();   
  
  Expr expr = new_expr();
  parser->dummies++;

  if (!parse_expr(parser, assoc, &rhs)) return 0;
  if (!is_expr_sort(rhs.typ)) return push_err(parser, "cannot from arrow of non-sort rhs term");

  expr.typ = EXP_FREE;
  expr.annot.typ = ANN_TYPED;
  format_to("$%d", expr.free, BUF_LEN, parser->dummies-1);
  expr.annot.ann = lhs;
  parser->dummies--;

  res->typ = EXP_PI;
  res->pi.lhs = expr_alloc(&expr);
  res->pi.rhs = expr_alloc(&rhs);
  res->pi.dep = 0;
  return 1;
}

int parse_annot (Parser* parser, Assoc assoc, Expr* lhs) {
  if (!is_expr_term(lhs->typ)) return push_err(parser, "expected term on lhs of annotation");

  Expr rhs = new_expr();
  if (!parse_expr(parser, assoc, &rhs)) return 0;
  if (!is_expr_sort(rhs.typ)) return push_err(parser, "expected sort on rhs on annotation");
  
  lhs->annot.typ = ANN_TYPED;
  lhs->annot.ann = expr_alloc(&rhs);
 
  return 1;
}

int parse_lam (Parser* parser, Assoc assoc, Expr* res) { 
  parser->binding = 1;
  Expr bind = new_expr();
  if (!parse_expr(parser, new_assoc(RASSOC, 0), &bind)) return 0;
  if (!is_expr_term(bind.typ)) return push_err(parser, "binding non-term as lambda parameter");
  if (bind.annot.typ == ANN_EMPTY) return push_err(parser, "binding expects annotation");

  push_bind(parser, &bind);

  Expr body = new_expr();
  if (try_eat(parser, TOK_PERIOD)) {
    parser->binding = 0;
    if (!parse_expr(parser, assoc, &body)) return 0;
  } else {
    if (!parse_lam(parser, assoc, &body)) return 0;
  }

  rem_bind(parser, &bind);
  
  res->typ = EXP_LAM;
  res->lam.lhs = expr_alloc(&bind);
  res->lam.rhs = expr_alloc(&body);
  return 1;
}

int parse_pi (Parser* parser, Assoc assoc, Expr* res) { 
  parser->binding = 1;
  Expr bind = new_expr();
  if (!parse_expr(parser, new_assoc(RASSOC, 0), &bind)) return 0;
  if (!is_expr_term(bind.typ)) return push_err(parser, "binding non-term as lambda parameter");

  push_bind(parser, &bind);

  Expr body = new_expr();
  if (try_eat(parser, TOK_PERIOD)) {
    parser->binding = 0;
    if (!parse_expr(parser, assoc, &body)) return 0;
  } else {
    if (!parse_pi(parser, assoc, &body)) return 0;
  }

  rem_bind(parser, &bind);
  
  res->typ = EXP_PI;
  res->pi.lhs = expr_alloc(&bind);
  res->pi.rhs = expr_alloc(&body);
  res->pi.dep = 1;
  return 1;
}

void print_expr (Expr* expr) {
  switch (expr->typ) {
    case EXP_APP:
      printf("(");
      print_expr(expr->app.lhs);
      printf(" ");
      print_expr(expr->app.rhs);
      printf(")");
      break;
    case EXP_LAM:
      printf("(λ");
      print_expr(expr->lam.lhs);
      printf(".");
      print_expr(expr->lam.rhs);
      printf(")");
      break;
    case EXP_PI:
      if (expr->pi.dep) {
        printf("(∀");
        print_expr(expr->pi.lhs);
        printf(". ");
        print_expr(expr->pi.rhs);
        printf(")");
      } else {
        printf("(");
        print_expr(expr->pi.lhs->annot.ann);
        printf("->");
        print_expr(expr->pi.rhs);
        printf(")");
      }
      break;
    case EXP_KIND:
      printf("*");
      break;
    case EXP_TERM: 
      //printf("%s", expr->term.name);
      printf("#%d", expr->term.idx);
      break;
    case EXP_FREE:
      printf("%s", expr->free);
      break;
  }  
  if (expr->annot.typ == ANN_TYPED) {
    printf(": ");
    print_expr(expr->annot.ann);
  }
}
