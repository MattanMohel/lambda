#include "parser.h"
#include "strop.h"
#include <stdarg.h>

typedef struct Parser {
  VEC(Token) stream;
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
  expr.type = NULL;
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

Assoc expr_assoc (TokenType type) {
  switch (type) {
    case TOK_APPL: return new_assoc(LASSOC, 50);
    case TOK_ABST: return new_assoc(LASSOC, 30);
    case TOK_TYPE: return new_assoc(LASSOC, 60);
    case TOK_FORALL: return new_assoc(LASSOC, 70);
    default: return new_assoc(RASSOC, -1);
  }
}

int parse (VEC(Token) tokens, Expr* res) {
  Parser state;
  memset(state.mes, '\0', BUF_LEN);
  state.stream = tokens;
  state.ptr = tokens;

  int pass = parse_expr(&state, new_assoc(RASSOC, 0), res);

  if (!pass) {
    printf("%s at (%d, %d)\n", state.mes, state.ptr->col, state.ptr->row);
  }
  return pass;
}

int parse_abst (Parser* parser, Expr* res) { 
  Expr bind = new_expr();
  if (!parse_expr(parser, new_assoc(RASSOC, 0), &bind)) return 0;
  if (bind.vrt != EXP_TERM) {
    push_err(parser, "binding non-term as lambda parameter");
    return 0;
  }

  res->vrt = EXP_ABST;
  res->abst.lhs = expr_alloc(&bind);

  Expr body = new_expr();
  switch (at(parser)->type) {
    case TOK_BODY: {
      eat(parser);      
      if (!parse_expr(parser, expr_assoc(TOK_ABST), &body)) return 0;
      break;
    }
    default: {
      if (!parse_abst(parser, &body)) return 0;
      break;
    }
  }

  res->abst.rhs = expr_alloc(&body);
  return 1;
}

int parse_poly (Parser* parser, Expr* res) { 
  Type bind;
  if (!parse_type(parser, new_assoc(RASSOC, 0), &bind)) return 0;
  if (bind.vrt != TYP_TERM) {
    push_err(parser, "binding non-term as type parameter");
    return 0;
  }

  // TODO: move allocation to post-failure point
  res->vrt = EXP_ABST_TYPE;
  res->abst_type.lhs = type_alloc(&bind);

  Expr body = new_expr();
  switch (at(parser)->type) {
    case TOK_BODY: {
      eat(parser);      
      if (!parse_expr(parser, expr_assoc(TOK_ABST), &body)) return 0;
      break;
    }
    default: {
      if (!parse_poly(parser, &body)) return 0;
      break;
    }
  }

  res->abst_type.rhs = expr_alloc(&body);
  return 1;
}

int parse_appl (Parser* parser, Assoc assoc, Expr* lhs, Expr* res) {
  if (expect(parser, TOK_LBRACKET)) {
    eat(parser);
    Type rhs;
    if (!parse_type(parser, new_assoc(RASSOC, 0), &rhs)) return 0;

    res->vrt = EXP_APPL_TYPE;
    res->appl_type.lhs = lhs;
    res->appl_type.rhs = type_alloc(&rhs);
    return try_eat(parser, TOK_RBRACKET);
  } 

  Expr rhs = new_expr(); 
  if (!parse_expr(parser, assoc, &rhs)) return 0;

  res->vrt = EXP_APPL;
  res->appl.lhs = lhs;
  res->appl.rhs = expr_alloc(&rhs);
  return 1;
}

int parse_expr (Parser* parser, Assoc assoc, Expr* res) {
  Expr lhs = new_expr(); 

  switch (at(parser)->type) {
    case TOK_TERM: {
      Token* cur = eat(parser);
      lhs.vrt = EXP_TERM;
      lhs.col = cur->col;
      lhs.row = cur->row;
      lhs.type = NULL;
      strcpy(lhs.term, cur->tok);
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
  if (!is_infix(op) || expr_assoc(op).bp < assoc_bp(assoc)) {
    memcpy(res, lhs, sizeof(Expr));
    free(lhs);
    return 1;
  }

  eat(parser);
  
  switch (op) {
    case TOK_APPL: {
      Expr infix = new_expr(); 
      if (!parse_appl(parser, expr_assoc(op), lhs, &infix)) return 0;
      
      Expr* alloc = expr_alloc(&infix);
      if (!parse_expr_infix(parser, assoc, alloc, res)) {
        free(alloc);
        return 0;
      } else {
        return 1;
      }
    }
    case TOK_TYPE: {
      if (lhs->vrt != EXP_TERM) return push_err(parser, "trying to type a non-term expression");

      Type* type = malloc(sizeof(Type));
      type->det = 1;
      // TODO: add fail condition here!
      parse_type(parser, new_assoc(RASSOC, 0), type);
      lhs->type = type;

      if (!parse_expr_infix(parser, assoc, lhs, res)) {
        free(type);
        return 0;
      } else {
        return 1;
      }
    }

    default: return push_err(parser, "expected infix, found '%s'", tok(op));
  }
}

int parse_expr_prefix (Parser* parser, Expr* res) {
  TokenType op = eat(parser)->type;
  
  switch (op) {
    case TOK_ABST: return parse_abst(parser, res);
    case TOK_FORALL: return parse_poly(parser, res);
    default: return push_err(parser, "expected prefix, found '%s'", tok(op));
  }
}

int parse_arrow (Parser* parser, Assoc assoc, Type* lhs, Type* res) {
  Type* rhs = malloc(sizeof(Expr)); 
  if (!parse_type(parser, assoc, rhs)) {
    free(rhs);
    return 0;
  }

  res->vrt = TYP_COMP;
  res->comp.lhs = lhs;
  res->comp.rhs = rhs;
  res->det = 0;

  return 1;
}

int parse_type (Parser* parser, Assoc assoc, Type* res) {
  Type lhs; 

  switch (at(parser)->type) {
    case TOK_TERM: {
      Token* cur = eat(parser);
      lhs.vrt = TYP_TERM;
      lhs.col = cur->col;
      lhs.row = cur->row;
      lhs.det = 1;
      strcpy(lhs.term, cur->tok);
      break;
    }
    case TOK_LPARENTH: {
      eat(parser);
      if (!parse_type(parser, new_assoc(RASSOC, 0), &lhs)) return 0;
      if (!try_eat(parser, TOK_RPARENTH)) return 0;
      break;
    }
    default: {
      if (!parse_type_prefix(parser, res)) return 0;
      break;
    }
  }

  Type* alloc = type_alloc(&lhs);
  if (!parse_type_infix(parser, assoc, alloc, res)) {
    free(alloc);
    return 0;
  }

  return 1;
}

int parse_type_infix (Parser* parser, Assoc assoc, Type* lhs, Type* res) { 
  TokenType op = at(parser)->type; 
  if (op != TOK_ARROW || type_assoc(op).bp < assoc_bp(assoc)) {
    memcpy(res, lhs, sizeof(Expr));
    return 1;
  }

  eat(parser);
  
  switch (op) {
    case TOK_ARROW: { 
      Type infix; 
      if (!parse_arrow(parser, expr_assoc(op), lhs, &infix)) return 0;
      
      Type* alloc = type_alloc(&infix);
      if (!parse_type_infix(parser, assoc, alloc, res)) {
        free(alloc);
        return 0;
      } else {
        return 1;
      }
    }
    default: {
      push_err(parser, "expected type infix, found '%s'", tok(op));
      return 0;
    }
  }
}

int parse_type_prefix (Parser* parser, Type* res) {
  return 0;
}

Assoc type_assoc (TokenType typ) {
  switch (typ) {
    case TOK_TYPE:  return new_assoc(LASSOC, 10);
    case TOK_ARROW: return new_assoc(RASSOC, 20);
    default: return new_assoc(RASSOC, -1);
  }
}

void print_type (Type* type) {
  switch (type->vrt) {
    case TYP_TERM:
      printf("%s", type->term);
      break;
    case TYP_COMP:
      printf("(");
      print_type(type->comp.lhs);
      printf("->");
      print_type(type->comp.rhs);
      printf(")");
      break;
    case TYP_POLY:
      printf("forall ");
      print_type(type->poly.lhs);
      printf(". ");
      print_type(type->poly.rhs);
      break;
  }
}

void print_expr (Expr* expr) {
  switch (expr->vrt) {
    case EXP_APPL:
      printf("(");
      print_expr(expr->appl.lhs);
      printf(" ");
      print_expr(expr->appl.rhs);
      printf(")");
      break;
    case EXP_ABST:
      printf("(λ");
      print_expr(expr->abst.lhs);
      printf(".");
      print_expr(expr->abst.rhs);
      printf(")");
      break;
    case EXP_ABST_TYPE:
      printf("(∀");
      print_type(expr->abst_type.lhs);
      printf(": ");
      print_expr(expr->abst_type.rhs);
      printf(")");
      break;
    case EXP_APPL_TYPE:
      printf("(");
      print_expr(expr->appl_type.lhs);
      printf(" [");
      print_type(expr->appl_type.rhs);
      printf("])");
      break;
    case EXP_TERM:      
      if (expr->type != NULL) {
        printf("[%s:", expr->term);
        print_type(expr->type);
        printf("]");
      } else {
        printf("%s", expr->term);
      }
      break;
  }
}

