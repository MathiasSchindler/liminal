#define _POSIX_C_SOURCE 200809L
#include "liminal/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} memset(p,0,n); return p; }

struct Parser {
  Lexer *lx;
  Token current;
  Token lookahead;
  int has_lookahead;
  ParseErrorVec errors;
  const char *src;
  size_t len;
};

static char *sdup(const char *s, size_t n) { char *p = malloc(n + 1); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} memcpy(p, s, n); p[n] = '\0'; return p; }

static Token peek_token(Parser *p) {
  if (!p->has_lookahead) {
    p->lookahead = lexer_next(p->lx);
    p->has_lookahead = 1;
  }
  return p->lookahead;
}

static Token consume_token(Parser *p) {
  Token t;
  if (p->has_lookahead) {
    t = p->lookahead;
    p->has_lookahead = 0;
  } else {
    t = lexer_next(p->lx);
  }
  p->current = t;
  return t;
}

static int token_is(Token t, TokenKind k) { return t.kind == k; }

static void add_error(Parser *p, LiminalSpan span, const char *msg) {
  if (p->errors.len == p->errors.cap) {
    p->errors.cap = p->errors.cap ? p->errors.cap * 2 : 4;
    p->errors.items = realloc(p->errors.items, p->errors.cap * sizeof(ParseError));
  }
  p->errors.items[p->errors.len].span = span;
  size_t n = strlen(msg);
  p->errors.items[p->errors.len].message = (char *)xmalloc(n + 1);
  memcpy(p->errors.items[p->errors.len].message, msg, n + 1);
  p->errors.len++;
}

static int match(Parser *p, TokenKind k) {
  Token t = peek_token(p);
  if (t.kind == k) { consume_token(p); return 1; }
  return 0;
}

static Token expect(Parser *p, TokenKind k, const char *msg) {
  Token t = peek_token(p);
  if (t.kind != k) {
    add_error(p, t.span, msg);
    return consume_token(p);
  }
  return consume_token(p);
}

// Forward decls
static ASTNode *parse_program_internal(Parser *p);
static ASTExpr *parse_expression(Parser *p, int prec);
static ASTStmt *parse_statement(Parser *p);
static ASTType *parse_type(Parser *p);
static ASTExpr *parse_fstring_token(Parser *p, Token t);
static ASTStmt *parse_for(Parser *p, Token for_tok);
static ASTStmt *parse_repeat(Parser *p);
static ASTStmt *parse_case(Parser *p);
static ASTExpr *make_concat(ASTExpr *lhs, ASTExpr *rhs);

static int precedence(TokenKind k) {
  switch (k) {
  case TK_OR: return 1;
  case TK_AND: return 2;
  case TK_EQ: case TK_NEQ: case TK_LT: case TK_GT: case TK_LE: case TK_GE: return 3;
  case TK_PLUS: case TK_MINUS: return 4;
  case TK_STAR: case TK_SLASH: case TK_DIV: case TK_MOD: return 5;
  default: return 0;
  }
}

static ASTExpr *make_binary(ASTExpr *lhs, Token op, ASTExpr *rhs) {
  ASTExpr *e = xmalloc(sizeof(ASTExpr));
  e->kind = EXPR_BINARY;
  e->span = lhs->span;
  e->as.binary.op = op.kind;
  e->as.binary.lhs = lhs;
  e->as.binary.rhs = rhs;
  return e;
}

static ASTExpr *make_concat(ASTExpr *lhs, ASTExpr *rhs) {
  ASTExpr *e = xmalloc(sizeof(ASTExpr));
  e->kind = EXPR_CONCAT;
  e->span = lhs ? lhs->span : rhs->span;
  e->as.concat.lhs = lhs;
  e->as.concat.rhs = rhs;
  return e;
}

static ASTExpr *parse_fstring_expr(const char *src, size_t len) {
  Parser *p2 = parser_create(src, len);
  ASTExpr *expr = parse_expression(p2, 0);
  parser_destroy(p2);
  return expr;
}

static ASTExpr *parse_fstring_token(Parser *p, Token t) {
  (void)p;
  const char *lex = t.lexeme;
  size_t len = t.lexeme_len;
  if (len < 3) return ast_make_literal(t);
  size_t i = 1; // skip leading f
  char quote = lex[i];
  if (quote != '\'' && quote != '"') return ast_make_literal(t);
  i++; // move past quote
  ASTExpr *acc = NULL;
  size_t start = i;
  while (i < len) {
    if (lex[i] == quote && i + 1 == len) {
      // trailing literal
      if (i > start) {
        size_t seglen = i - start;
        char *buf = malloc(seglen + 3);
        buf[0] = '\''; memcpy(buf+1, lex+start, seglen); buf[seglen+1] = '\''; buf[seglen+2] = '\0';
        Token lit = t; lit.kind = TK_STRING; lit.lexeme = buf; lit.lexeme_len = (int)(seglen + 2);
        ASTExpr *litexpr = ast_make_literal(lit);
        free(buf);
        acc = acc ? make_concat(acc, litexpr) : litexpr;
      }
      break;
    }
    if (lex[i] == '{') {
      // literal before
      if (i > start) {
        size_t seglen = i - start;
        char *buf = malloc(seglen + 3);
        buf[0] = '\''; memcpy(buf+1, lex+start, seglen); buf[seglen+1] = '\''; buf[seglen+2] = '\0';
        Token lit = t; lit.kind = TK_STRING; lit.lexeme = buf; lit.lexeme_len = (int)(seglen + 2);
        ASTExpr *litexpr = ast_make_literal(lit);
        free(buf);
        acc = acc ? make_concat(acc, litexpr) : litexpr;
      }
      i++;
      size_t expr_start = i;
      while (i < len && lex[i] != '}') i++;
      size_t expr_len = i - expr_start;
      ASTExpr *expr = parse_fstring_expr(lex + expr_start, expr_len);
      acc = acc ? make_concat(acc, expr) : expr;
      i++; // skip '}'
      start = i;
      continue;
    }
    i++;
  }
  return acc ? acc : ast_make_literal(t);
}

static ASTExpr *parse_primary(Parser *p) {
  Token t = peek_token(p);
  switch (t.kind) {
  case TK_IDENTIFIER:
  case TK_KEYWORD: {
    // handle True/False literals earlier
    if (t.kind==TK_KEYWORD && t.lexeme_len==6 && strncasecmp(t.lexeme, "repeat", 6)==0) {
      consume_token(p);
      // push back a repeat stmt? Not here; let statement parser consume
      return ast_make_ident(t);
    }
    if (t.kind==TK_KEYWORD && t.lexeme_len==5 && strncasecmp(t.lexeme, "until", 5)==0) {
      consume_token(p);
      return ast_make_ident(t);
    }
    // ask / embed keywords
    if (t.kind == TK_KEYWORD && t.lexeme_len==3 && strncasecmp(t.lexeme, "ask", 3)==0) {
      consume_token(p);
      Token oracle = expect(p, TK_IDENTIFIER, "Expected oracle after ask");
      if (match(p, TK_LT)) {
        if (peek_token(p).kind == TK_MINUS) consume_token(p);
      } else {
        add_error(p, peek_token(p).span, "Expected <- after ask oracle");
      }
      ASTExpr *input = parse_expression(p, 0);
      ASTExpr *oracle_expr = ast_make_ident(oracle);
      ASTExpr *e = xmalloc(sizeof(ASTExpr));
      e->kind = EXPR_ASK;
      e->span = t.span;
      e->as.ask.oracle = oracle_expr;
      e->as.ask.input = input;
      e->as.ask.into_type = NULL;
      e->as.ask.timeout = NULL;
      e->as.ask.fallback = NULL;
      e->as.ask.with_cost = 0;
      // optional into schema
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "into", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        e->as.ask.into_type = parse_type(p);
      }
      // optional else fallback
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "else", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        e->as.ask.fallback = parse_expression(p, 0);
      }
      // optional with cost
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "with", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "cost", peek_token(p).lexeme_len)==0) {
          consume_token(p);
          e->as.ask.with_cost = 1;
        }
      }
      return e;
    }
    if (t.kind == TK_KEYWORD && t.lexeme_len==7 && strncasecmp(t.lexeme, "consult", 7)==0) {
      consume_token(p);
      Token oracle = expect(p, TK_IDENTIFIER, "Expected oracle after consult");
      if (!(token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "from", peek_token(p).lexeme_len)==0)) {
        add_error(p, peek_token(p).span, "Expected from after consult oracle");
      } else {
        consume_token(p);
      }
      ASTExpr *input = parse_expression(p, 0);
      ASTExpr *oracle_expr = ast_make_ident(oracle);
      ASTExpr *e = xmalloc(sizeof(ASTExpr));
      e->kind = EXPR_CONSULT;
      e->span = t.span;
      e->as.consult.oracle = oracle_expr;
      e->as.consult.input = input;
      e->as.consult.into_type = NULL;
      e->as.consult.attempts = 1;
      e->as.consult.hint = NULL;
      e->as.consult.fallback = NULL;
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "into", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        e->as.consult.into_type = parse_type(p);
      }
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "with", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        if ((token_is(peek_token(p), TK_IDENTIFIER) || token_is(peek_token(p), TK_KEYWORD)) && strncasecmp(peek_token(p).lexeme, "attempts", peek_token(p).lexeme_len)==0) {
          consume_token(p);
          expect(p, TK_COLON, "Expected : after attempts");
          Token num = expect(p, TK_INTEGER, "Expected attempts integer");
          e->as.consult.attempts = atoi(num.lexeme);
        }
      }
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "on", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "failure", peek_token(p).lexeme_len)==0) {
          consume_token(p);
          // optional (F: TOracleFailure)
          if (match(p, TK_LPAREN)) {
            // consume until )
            while (!match(p, TK_RPAREN) && peek_token(p).kind != TK_EOF) consume_token(p);
          }
          // actions until end
          while (!(token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
            Token nt = peek_token(p);
            if (token_is(nt, TK_KEYWORD) && strncasecmp(nt.lexeme, "retry", nt.lexeme_len)==0) {
              consume_token(p);
              if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "with", peek_token(p).lexeme_len)==0) {
                consume_token(p);
                if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "hint", peek_token(p).lexeme_len)==0) {
                  consume_token(p);
                  e->as.consult.hint = parse_expression(p, 0);
                }
              }
            } else if (token_is(nt, TK_KEYWORD) && strncasecmp(nt.lexeme, "yield", nt.lexeme_len)==0) {
              consume_token(p);
              e->as.consult.fallback = parse_expression(p, 0);
            } else {
              // skip statement (not implemented)
              consume_token(p);
            }
            match(p, TK_SEMICOLON);
          }
          expect(p, TK_KEYWORD, "Expected end");
        }
      }
      // optional else fallback (simple syntax)
      if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "else", peek_token(p).lexeme_len)==0) {
        consume_token(p);
        e->as.consult.fallback = parse_expression(p, 0);
      }
      return e;
    }
    if (t.kind == TK_KEYWORD && t.lexeme_len==5 && strncasecmp(t.lexeme, "embed", 5)==0) {
      consume_token(p);
      Token oracle = expect(p, TK_IDENTIFIER, "Expected oracle after embed");
      expect(p, TK_LT, "Expected <-");
      ASTExpr *input = parse_expression(p, 0);
      ASTExpr *oracle_expr = ast_make_ident(oracle);
      ASTExpr *e = xmalloc(sizeof(ASTExpr));
      e->kind = EXPR_EMBED;
      e->span = t.span;
      e->as.embed.oracle = oracle_expr;
      e->as.embed.input = input;
      return e;
    }
    if (t.kind == TK_KEYWORD && t.lexeme_len==4 && strncasecmp(t.lexeme,"true",4)==0) {
      consume_token(p);
      Token tb = t; tb.kind = TK_BOOL;
      return ast_make_literal(tb);
    }
    if (t.kind == TK_KEYWORD && t.lexeme_len==5 && strncasecmp(t.lexeme,"false",5)==0) {
      consume_token(p);
      Token tb = t; tb.kind = TK_BOOL;
      return ast_make_literal(tb);
    }
    consume_token(p);
    return ast_make_ident(t);
  }
  case TK_INTEGER:
  case TK_REAL:
  case TK_STRING:
  case TK_BYTES:
  case TK_CHAR:
  case TK_FSTRING:
    consume_token(p);
    if (t.kind == TK_FSTRING) return parse_fstring_token(p, t);
    return ast_make_literal(t);
  case TK_DURATION:
  case TK_MONEY:
    consume_token(p);
    if (t.kind == TK_FSTRING) return parse_fstring_token(p, t);
    return ast_make_literal(t);
  case TK_EOF:
    add_error(p, t.span, "Unexpected EOF");
    consume_token(p);
    return ast_make_ident(t);
  case TK_LPAREN: {
    consume_token(p);
    ASTExpr *first = parse_expression(p, 0);
    if (match(p, TK_COMMA)) {
      ASTExpr *e = xmalloc(sizeof(ASTExpr));
      e->kind = EXPR_TUPLE;
      e->span = first->span;
      ast_expr_vec_push(&e->as.tuple.elements, first);
      do {
        ASTExpr *next = parse_expression(p, 0);
        ast_expr_vec_push(&e->as.tuple.elements, next);
      } while (match(p, TK_COMMA));
      expect(p, TK_RPAREN, "Expected ) to close tuple");
      return e;
    } else {
      expect(p, TK_RPAREN, "Expected )");
      return first;
    }
  }
  case TK_LBRACKET: {
    consume_token(p);
    ASTExpr *e = xmalloc(sizeof(ASTExpr));
    e->kind = EXPR_ARRAY;
    e->span = t.span;
    if (!match(p, TK_RBRACKET)) {
      do {
        ASTExpr *elem = parse_expression(p, 0);
        ast_expr_vec_push(&e->as.array.elements, elem);
      } while (match(p, TK_COMMA));
      expect(p, TK_RBRACKET, "Expected ]");
    }
    return e;
  }
  case TK_LBRACE: {
    consume_token(p);
    ASTExpr *e = xmalloc(sizeof(ASTExpr));
    e->kind = EXPR_RECORD;
    e->span = t.span;
    if (!match(p, TK_RBRACE)) {
      do {
        Token key = expect(p, TK_IDENTIFIER, "Expected field name");
        expect(p, TK_COLON, "Expected : in record field");
        ASTExpr *val = parse_expression(p, 0);
        ASTField f = { .key = {0}, .value = val };
        f.key.data = sdup(key.lexeme, key.lexeme_len);
        f.key.len = key.lexeme_len;
        ast_field_vec_push(&e->as.record.fields, f);
      } while (match(p, TK_COMMA));
      expect(p, TK_RBRACE, "Expected }");
    }
    return e;
  }
  default:
    consume_token(p);
    add_error(p, t.span, "Unexpected token in expression");
    return ast_make_literal(t);
  }
}

static ASTExpr *parse_postfix(Parser *p) {
  ASTExpr *expr = parse_primary(p);
  for (;;) {
    Token t = peek_token(p);
    if (t.kind == TK_DOT) {
      consume_token(p);
      Token field = expect(p, TK_IDENTIFIER, "Expected field name after .");
      ASTExpr *e = xmalloc(sizeof(ASTExpr));
      e->kind = EXPR_FIELD;
      e->span = expr->span;
      e->as.field.base = expr;
      e->as.field.field.data = sdup(field.lexeme, field.lexeme_len);
      e->as.field.field.len = field.lexeme_len;
      expr = e;
    } else if (t.kind == TK_LPAREN) {
      consume_token(p);
      ASTExpr *call = xmalloc(sizeof(ASTExpr));
      call->kind = EXPR_CALL;
      call->as.call.callee = expr;
      call->span = expr->span;
      if (!match(p, TK_RPAREN)) {
        do {
          ASTExpr *arg = parse_expression(p, 0);
          ast_expr_vec_push(&call->as.call.args, arg);
        } while (match(p, TK_COMMA));
        expect(p, TK_RPAREN, "Expected )");
      }
      expr = call;
    } else if (t.kind == TK_LBRACKET) {
      consume_token(p);
      ASTExpr *idx = xmalloc(sizeof(ASTExpr));
      idx->kind = EXPR_INDEX;
      idx->as.index.base = expr;
      idx->span = expr->span;
      ASTExpr *first = parse_expression(p, 0);
      ast_expr_vec_push(&idx->as.index.indices, first);
      while (match(p, TK_COMMA)) {
        ASTExpr *next = parse_expression(p, 0);
        ast_expr_vec_push(&idx->as.index.indices, next);
      }
      expect(p, TK_RBRACKET, "Expected ]");
      expr = idx;
    } else {
      break;
    }
  }
  return expr;
}

static ASTExpr *parse_unary(Parser *p) {
  Token t = peek_token(p);
  if (t.kind == TK_MINUS || t.kind == TK_NOT) {
    consume_token(p);
    ASTExpr *operand = parse_unary(p);
    ASTExpr *e = xmalloc(sizeof(ASTExpr));
    e->kind = EXPR_UNARY;
    e->span = t.span;
    e->as.unary.op = t.kind;
    e->as.unary.expr = operand;
    return e;
  }
  return parse_postfix(p);
}

static ASTExpr *parse_expression(Parser *p, int prec) {
  ASTExpr *lhs = parse_unary(p);
  for (;;) {
    Token op = peek_token(p);
    int op_prec = precedence(op.kind);
    if (op_prec == 0 || op_prec <= prec) break;
    consume_token(p);
    ASTExpr *rhs = parse_expression(p, op_prec);
    lhs = make_binary(lhs, op, rhs);
  }
  return lhs;
}

static ASTStmt *make_stmt(StmtKind k, LiminalSpan span) { ASTStmt *s = xmalloc(sizeof(ASTStmt)); s->kind = k; s->span = span; return s; }

static ASTStmt *parse_if(Parser *p) {
  Token ift = expect(p, TK_KEYWORD, "Expected if");
  ASTExpr *cond = parse_expression(p, 0);
  expect(p, TK_KEYWORD, "Expected then");
  ASTStmt *then_branch = parse_statement(p);
  ASTStmt *else_branch = NULL;
  if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "else", peek_token(p).lexeme_len)==0) {
    consume_token(p);
    else_branch = parse_statement(p);
  }
  ASTStmt *s = make_stmt(STMT_IF, ift.span);
  s->as.if_stmt.cond = cond;
  s->as.if_stmt.then_branch = then_branch;
  s->as.if_stmt.else_branch = else_branch;
  return s;
}

static ASTStmt *parse_while(Parser *p) {
  Token wt = consume_token(p);
  ASTExpr *cond = parse_expression(p, 0);
  expect(p, TK_KEYWORD, "Expected do");
  ASTStmt *body = parse_statement(p);
  ASTStmt *s = make_stmt(STMT_WHILE, wt.span);
  s->as.while_stmt.cond = cond;
  s->as.while_stmt.body = body;
  return s;
}

static ASTStmt *parse_repeat(Parser *p) {
  Token rpt = expect(p, TK_KEYWORD, "Expected repeat");
  ASTStmt *s = make_stmt(STMT_REPEAT, rpt.span);
  ASTStmtVec stmts = {0};
  // repeat ... until ...; allow semicolons between statements
  while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "until", peek_token(p).lexeme_len)==0)) {
    ASTStmt *st = parse_statement(p);
    ast_stmt_vec_push(&stmts, st);
    match(p, TK_SEMICOLON);
  }
  expect(p, TK_KEYWORD, "Expected until");
  s->as.repeat_stmt.body = ast_make_block(&stmts, rpt.span);
  s->as.repeat_stmt.cond = parse_expression(p, 0);
  return s;
}
static ASTStmt *parse_case(Parser *p) {
  Token ct = consume_token(p); // case
  ASTStmt *s = make_stmt(STMT_CASE, ct.span);
  s->as.case_stmt.expr = parse_expression(p, 0);
  expect(p, TK_KEYWORD, "Expected of");
  while (!(peek_token(p).kind==TK_KEYWORD && strncasecmp(peek_token(p).lexeme,"end", peek_token(p).lexeme_len)==0)) {
    if (peek_token(p).kind==TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "else", peek_token(p).lexeme_len)==0) { consume_token(p); s->as.case_stmt.else_branch = parse_statement(p); break; }
    ASTExpr *pat = parse_expression(p, 0);
    expect(p, TK_COLON, "Expected :");
    ASTStmt *br = parse_statement(p);
    ast_expr_vec_push(&s->as.case_stmt.patterns, pat);
    ast_stmt_vec_push(&s->as.case_stmt.branches, br);
    match(p, TK_SEMICOLON);
  }
  expect(p, TK_KEYWORD, "Expected end");
  return s;
}

static ASTStmt *parse_for(Parser *p, Token for_tok) {
  consume_token(p); // consume 'for'
  Token var = expect(p, TK_IDENTIFIER, "Expected loop variable");
  if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "in", peek_token(p).lexeme_len)==0) {
    consume_token(p); // in
    ASTExpr *iter = parse_expression(p, 0);
    expect(p, TK_KEYWORD, "Expected do");
    ASTStmt *body = parse_statement(p);
    ASTStmt *s = make_stmt(STMT_FOR_IN, for_tok.span);
    s->as.for_in_stmt.var.name.data = sdup(var.lexeme, var.lexeme_len);
    s->as.for_in_stmt.var.name.len = var.lexeme_len;
    s->as.for_in_stmt.iterable = iter;
    s->as.for_in_stmt.body = body;
    return s;
  }
  expect(p, TK_ASSIGN, "Expected :=");
  ASTExpr *init = parse_expression(p, 0);
  Token dir = expect(p, TK_KEYWORD, "Expected to/downto");
  int descending = (strncasecmp(dir.lexeme, "downto", dir.lexeme_len)==0);
  ASTExpr *limit = parse_expression(p, 0);
  expect(p, TK_KEYWORD, "Expected do");
  ASTStmt *body = parse_statement(p);
  ASTStmt *s = make_stmt(STMT_FOR, for_tok.span);
  s->as.for_stmt.init = init;
  s->as.for_stmt.to = limit;
  s->as.for_stmt.body = body;
  s->as.for_stmt.var.name.data = sdup(var.lexeme, var.lexeme_len);
  s->as.for_stmt.var.name.len = var.lexeme_len;
  s->as.for_stmt.descending = descending;
  return s;
}

static ASTStmt *parse_assignment_or_expr(Parser *p) {
  ASTExpr *lhs = parse_expression(p, 0);
  if (match(p, TK_ASSIGN)) {
    ASTExpr *rhs = parse_expression(p, 0);
    ASTStmt *s = make_stmt(STMT_ASSIGN, lhs->span);
    s->as.assign.target = lhs;
    s->as.assign.value = rhs;
    return s;
  }
  ASTStmt *s = make_stmt(STMT_EXPR, lhs->span);
  s->as.expr_stmt.expr = lhs;
  return s;
}

static ASTStmt *parse_statement(Parser *p) {
  Token t = peek_token(p);
  if (getenv("LIMINAL_DEBUG_PARSER")) fprintf(stderr, "[parser] stmt at %s\n", t.lexeme);
  if (t.kind == TK_KEYWORD) {
    if (strncasecmp(t.lexeme, "if", t.lexeme_len)==0) return parse_if(p);
    if (strncasecmp(t.lexeme, "while", t.lexeme_len)==0) return parse_while(p);
    if (strncasecmp(t.lexeme, "repeat", t.lexeme_len)==0) return parse_repeat(p);
    if (strncasecmp(t.lexeme, "for", t.lexeme_len)==0) return parse_for(p, t);
    if (strncasecmp(t.lexeme, "case", t.lexeme_len)==0) return parse_case(p);
    if (strncasecmp(t.lexeme, "break", t.lexeme_len)==0) { consume_token(p); return make_stmt(STMT_BREAK, t.span);} 
    if (strncasecmp(t.lexeme, "continue", t.lexeme_len)==0) { consume_token(p); return make_stmt(STMT_CONTINUE, t.span);} 
    if (strncasecmp(t.lexeme, "begin", t.lexeme_len)==0) {
      consume_token(p);
      ASTStmtVec stmts = {0};
      while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
        if (peek_token(p).kind == TK_EOF) { add_error(p, peek_token(p).span, "Unexpected EOF in block"); break; }
        ASTStmt *s = parse_statement(p);
        ast_stmt_vec_push(&stmts, s);
        match(p, TK_SEMICOLON);
      }
      expect(p, TK_KEYWORD, "Expected end");
      return ast_make_block(&stmts, t.span);
    }
  }
  return parse_assignment_or_expr(p);
}

static ASTType *make_type(TypeKind k, LiminalSpan span) {
  ASTType *t = xmalloc(sizeof(ASTType));
  t->kind = k;
  t->span = span;
  return t;
}

static ASTType *parse_primary_type(Parser *p) {
  Token t = peek_token(p);
  if (t.kind == TK_KEYWORD && strncasecmp(t.lexeme, "record", t.lexeme_len)==0) {
    consume_token(p);
    ASTType *ty = make_type(TYPE_RECORD, t.span);
    while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
      Token fname = expect(p, TK_IDENTIFIER, "Expected field name");
      expect(p, TK_COLON, "Expected :");
      ASTType *ft = parse_type(p);
      ASTFieldDecl fd = { .name = { .data = sdup(fname.lexeme, fname.lexeme_len), .len = fname.lexeme_len }, .type = ft, .describe = {0} };
      ast_field_decl_vec_push(&ty->as.record_type.fields, fd);
      expect(p, TK_SEMICOLON, "Expected ;");
    }
    expect(p, TK_KEYWORD, "Expected end");
    return ty;
  }
  if (t.kind == TK_IDENTIFIER || t.kind == TK_KEYWORD) {
    consume_token(p);
    // schema declaration handled elsewhere
    ASTType *ty = make_type(TYPE_IDENT, t.span);
    ty->as.ident.name = (String){ .data = sdup(t.lexeme, t.lexeme_len), .len = t.lexeme_len };
    return ty;
  }
  if (match(p, TK_LPAREN)) {
    // Could be tuple or enum
    ASTType *tuple = make_type(TYPE_TUPLE, t.span);
    int all_ident = 1;
    do {
      ASTType *elem = parse_type(p);
      if (elem->kind != TYPE_IDENT) all_ident = 0;
      ast_type_vec_push(&tuple->as.tuple_type.elements, elem);
    } while (match(p, TK_COMMA));
    expect(p, TK_RPAREN, "Expected ) in tuple type");
    if (all_ident) {
      ASTType *enm = make_type(TYPE_ENUM, t.span);
      for (size_t i=0;i<tuple->as.tuple_type.elements.len;++i){ ASTType *e = tuple->as.tuple_type.elements.items[i]; ast_enum_vec_push(&enm->as.enum_type, (String){ .data = sdup(e->as.ident.name.data, e->as.ident.name.len), .len = e->as.ident.name.len }); }
      ast_type_free(tuple);
      return enm;
    }
    return tuple;
  }
  if (match(p, TK_LBRACKET)) {
    // not in grammar; ignore
  }
  add_error(p, t.span, "Unexpected token in type");
  consume_token(p);
  return make_type(TYPE_IDENT, t.span);
}

static ASTType *parse_type(Parser *p) {
  // Handle optional prefix '?', result '!' suffix, array prefix 'array'
  Token t = peek_token(p);
  if (match(p, TK_QMARK)) {
    ASTType *opt = make_type(TYPE_OPTIONAL, t.span);
    opt->as.optional_type.option = parse_type(p);
    return opt;
  }
  if (match(p, TK_BANG)) {
    ASTType *res = make_type(TYPE_RESULT, t.span);
    res->as.result_type.ok = parse_type(p);
    res->as.result_type.err = NULL;
    return res;
  }
  if (t.kind == TK_KEYWORD && strncasecmp(t.lexeme, "array", t.lexeme_len)==0) {
    consume_token(p);
    ASTType *arr = make_type(TYPE_ARRAY, t.span);
    if (match(p, TK_LBRACKET)) {
      // length constraint array[1..5]
      Token minTok = peek_token(p);
      if (minTok.kind == TK_INTEGER) {
        consume_token(p);
        arr->as.array_type.has_len_min = 1;
        arr->as.array_type.len_min = atof(minTok.lexeme);
        if (match(p, TK_DOT_DOT)) {
          Token maxTok = expect(p, TK_INTEGER, "Expected max length");
          arr->as.array_type.has_len_max = 1;
          char tmp2[64]; snprintf(tmp2, sizeof(tmp2), "%.*s", (int)maxTok.lexeme_len, maxTok.lexeme);
          arr->as.array_type.len_max = atof(tmp2);
        }
      }
      expect(p, TK_RBRACKET, "Expected ]");
    }
    expect(p, TK_KEYWORD, "Expected of");
    arr->as.array_type.elem = parse_type(p);
    return arr;
  }
  ASTType *base = parse_primary_type(p);
  // constraints after base: [min..max], matching regex
  if (match(p, TK_LBRACKET)) {
    ASTType *ct = make_type(TYPE_CONSTRAINED, t.span);
    ct->as.constrained_type.base = (String){ .data = sdup(base->as.ident.name.data, base->as.ident.name.len), .len = base->as.ident.name.len };
    ast_type_free(base);
    Token minTok = peek_token(p);
    if (minTok.kind == TK_INTEGER) {
      consume_token(p);
      char tmp[64]; snprintf(tmp, sizeof(tmp), "%.*s", (int)minTok.lexeme_len, minTok.lexeme);
      ct->as.constrained_type.has_min = 1;
      ct->as.constrained_type.min = atof(tmp);
      if (match(p, TK_DOT_DOT)) {
        Token maxTok = expect(p, TK_INTEGER, "Expected max");
        ct->as.constrained_type.has_max = 1;
        ct->as.constrained_type.max = atof(maxTok.lexeme);
      }
    }
    expect(p, TK_RBRACKET, "Expected ]");
    // length constraint for String/array
    if (ct->as.constrained_type.base.data && strncasecmp(ct->as.constrained_type.base.data, "String", ct->as.constrained_type.base.len)==0)
      ct->as.constrained_type.length_constraint = 1;
    return ct;
  }
  // matching regex
  if (base && base->kind == TYPE_IDENT && base->as.ident.name.data && strncasecmp(base->as.ident.name.data, "String", base->as.ident.name.len)==0) {
    if (peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "matching", peek_token(p).lexeme_len)==0) {
      consume_token(p);
      Token pat = expect(p, TK_STRING, "Expected pattern string");
      ASTType *ct = make_type(TYPE_CONSTRAINED, t.span);
      ct->as.constrained_type.base = (String){ .data = sdup(base->as.ident.name.data, base->as.ident.name.len), .len = base->as.ident.name.len };
      ct->as.constrained_type.pattern = sdup(pat.lexeme+1, pat.lexeme_len-2);
      ast_type_free(base);
      return ct;
    }
  }
  return base;
}

static ASTNode *parse_function(Parser *p) {
  Token fn_tok = expect(p, TK_KEYWORD, "Expected function");
  if (getenv("LIMINAL_DEBUG_PARSER")) fprintf(stderr, "[parser] function\n");
  Token fname = expect(p, TK_IDENTIFIER, "Expected function name");
  expect(p, TK_LPAREN, "Expected (");
  ASTParamVec params = {0};
  if (!match(p, TK_RPAREN)) {
    for (;;) {
      // Parse a param group: a, b: Type
      String *names=NULL; size_t nlen=0,ncap=0;
      Token first = expect(p, TK_IDENTIFIER, "Expected param name");
      #define PUSH_PNAME(tok) do { if(nlen==ncap){ncap=ncap? ncap*2:4; names=realloc(names,ncap*sizeof(String));} names[nlen].data=sdup((tok).lexeme,(tok).lexeme_len); names[nlen].len=(tok).lexeme_len; nlen++; } while(0)
      PUSH_PNAME(first);
      while (match(p, TK_COMMA)) { Token nt=expect(p, TK_IDENTIFIER, "Expected param name"); PUSH_PNAME(nt); }
      #undef PUSH_PNAME
      expect(p, TK_COLON, "Expected :");
      ASTType *ptype = parse_type(p);
      for (size_t i=0;i<nlen;++i) {
        ASTParam param = { .name = names[i], .type = (i==0? ptype : ast_type_clone(ptype)) };
        ast_param_vec_push(&params, param);
      }
      free(names);
      if (match(p, TK_SEMICOLON)) continue;
      break;
    }
    expect(p, TK_RPAREN, "Expected )");
  }
  expect(p, TK_COLON, "Expected :");
  ASTType *result_type = parse_type(p);
  expect(p, TK_SEMICOLON, "Expected ;");
  // optional var block
  ASTVarDeclVec locals = {0};
  if (token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "var", peek_token(p).lexeme_len)==0) {
    consume_token(p);
    while (!(token_is(peek_token(p), TK_KEYWORD) && strncasecmp(peek_token(p).lexeme, "begin", peek_token(p).lexeme_len)==0)) {
      // Parse a var decl list: a, b: Type;
      String *names=NULL; size_t nlen=0,ncap=0;
      Token first = expect(p, TK_IDENTIFIER, "Expected var name");
      #define PUSH_NAME(tok) do { if(nlen==ncap){ncap=ncap? ncap*2:4; names=realloc(names,ncap*sizeof(String));} names[nlen].data=sdup((tok).lexeme,(tok).lexeme_len); names[nlen].len=(tok).lexeme_len; nlen++; } while(0)
      PUSH_NAME(first);
      while (match(p, TK_COMMA)) { Token nt=expect(p,TK_IDENTIFIER,"Expected var name"); PUSH_NAME(nt); }
      #undef PUSH_NAME
      expect(p, TK_COLON, "Expected :");
      ASTType *vtype = parse_type(p);
      ASTExpr *init = NULL;
      if (match(p, TK_ASSIGN)) init = parse_expression(p, 0);
      expect(p, TK_SEMICOLON, "Expected ;");
      for (size_t i=0;i<nlen;++i){
        ASTVarDecl vd = { .name = names[i], .type = (i==0? vtype : ast_type_clone(vtype)), .init = (init&&i==0)? init : (init? ast_expr_clone(init): NULL)};
        ast_var_decl_vec_push(&locals, vd);
      }
      free(names);
    }
    expect(p, TK_KEYWORD, "Expected begin");
  } else {
    expect(p, TK_KEYWORD, "Expected begin");
  }
  // body parsed as block until end
  ASTStmtVec stmts = {0};
  while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
    if (peek_token(p).kind == TK_EOF) { add_error(p, peek_token(p).span, "Unexpected EOF in function"); break; }
    ASTStmt *s = parse_statement(p);
    ast_stmt_vec_push(&stmts, s);
    match(p, TK_SEMICOLON);
  }
  expect(p, TK_KEYWORD, "Expected end");
  ASTStmt *body = ast_make_block(&stmts, fn_tok.span);
  expect(p, TK_SEMICOLON, "Expected ; after function");
  ASTNode *fn = xmalloc(sizeof(ASTNode));
  fn->kind = AST_FUNC_DECL;
  fn->span = fn_tok.span;
  fn->as.func_decl.name = (String){ .data = sdup(fname.lexeme, fname.lexeme_len), .len = fname.lexeme_len };
  fn->as.func_decl.params = params;
  fn->as.func_decl.result_type = result_type;
  fn->as.func_decl.body = body;
  if (locals.len > 0) { ASTVarBlock *blk = xmalloc(sizeof(ASTVarBlock)); blk->vars = locals; fn->as.func_decl.locals = blk; }
  else { fn->as.func_decl.locals = NULL; }
  return fn;
}

static __attribute__((unused)) ASTNode *parse_var_decl(Parser *p) {
  Token name = expect(p, TK_IDENTIFIER, "Expected var name");
  expect(p, TK_COLON, "Expected :");
  ASTType *type = parse_type(p);
  ASTExpr *init = NULL;
  if (match(p, TK_ASSIGN)) init = parse_expression(p, 0);
  expect(p, TK_SEMICOLON, "Expected ;");
  ASTNode *n = xmalloc(sizeof(ASTNode));
  n->kind = AST_VAR_DECL;
  n->span = name.span;
  n->as.var_decl.name = (String){ .data = sdup(name.lexeme, name.lexeme_len), .len = name.lexeme_len };
  n->as.var_decl.type = type;
  n->as.var_decl.init = init;
  return n;
}

static void parse_var_decl_into(Parser *p, ASTNodeVec *out) {
  // Parse one or more names separated by commas: a, b, c: Type;
  String *names = NULL; size_t nlen = 0, ncap = 0;
  Token first = expect(p, TK_IDENTIFIER, "Expected var name");
  #define PUSH_NAME(tok) do { \
    if (nlen == ncap) { ncap = ncap ? ncap * 2 : 4; names = realloc(names, ncap * sizeof(String)); } \
    names[nlen].data = sdup((tok).lexeme, (tok).lexeme_len); \
    names[nlen].len = (tok).lexeme_len; \
    nlen++; \
  } while (0)
  PUSH_NAME(first);
  while (match(p, TK_COMMA)) {
    Token nt = expect(p, TK_IDENTIFIER, "Expected var name");
    PUSH_NAME(nt);
  }
  #undef PUSH_NAME
  expect(p, TK_COLON, "Expected :");
  ASTType *type = parse_type(p);
  ASTExpr *init = NULL;
  if (match(p, TK_ASSIGN)) init = parse_expression(p, 0);
  expect(p, TK_SEMICOLON, "Expected ;");
  for (size_t i = 0; i < nlen; ++i) {
    ASTNode *n = xmalloc(sizeof(ASTNode));
    n->kind = AST_VAR_DECL;
    n->span = first.span;
    n->as.var_decl.name = names[i];
    n->as.var_decl.type = (i == 0 ? type : ast_type_clone(type));
    n->as.var_decl.init = (init && i == 0) ? init : (init ? ast_expr_clone(init) : NULL);
    ast_node_vec_push(out, n);
  }
  free(names);
}

static ASTType *parse_schema_type(Parser *p) {
  ASTType *t = make_type(TYPE_SCHEMA, peek_token(p).span);
  while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
    Token fname = expect(p, TK_IDENTIFIER, "Expected field name");
    expect(p, TK_COLON, "Expected :");
    ASTType *ftype = parse_type(p);
    String describe = {0};
    if (peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "describe", peek_token(p).lexeme_len)==0) {
      consume_token(p);
      Token d = expect(p, TK_STRING, "Expected description string");
      describe.data = sdup(d.lexeme+1, d.lexeme_len-2);
      describe.len = d.lexeme_len-2;
    }
    expect(p, TK_SEMICOLON, "Expected ;");
    ASTFieldDecl fd = { .name = { .data = sdup(fname.lexeme, fname.lexeme_len), .len = fname.lexeme_len }, .type = ftype, .describe = describe };
    ast_field_decl_vec_push(&t->as.schema_type.fields, fd);
  }
  expect(p, TK_KEYWORD, "Expected end");
  expect(p, TK_SEMICOLON, "Expected ; after schema");
  return t;
}

static ASTNode *parse_type_decl(Parser *p) {
  Token t = peek_token(p);
  if (t.kind == TK_KEYWORD && strncasecmp(t.lexeme, "schema", t.lexeme_len)==0) {
    consume_token(p);
    Token name = expect(p, TK_IDENTIFIER, "Expected schema name");
    ASTType *stype = parse_schema_type(p);
    ASTNode *n = xmalloc(sizeof(ASTNode));
    n->kind = AST_TYPE_DECL;
    n->span = name.span;
    n->as.type_decl.name = (String){ .data = sdup(name.lexeme, name.lexeme_len), .len = name.lexeme_len };
    n->as.type_decl.type = stype;
    return n;
  }
  Token name = expect(p, TK_IDENTIFIER, "Expected type name");
  expect(p, TK_EQ, "Expected = in type decl");
  ASTType *type = parse_type(p);
  expect(p, TK_SEMICOLON, "Expected ;");
  ASTNode *n = xmalloc(sizeof(ASTNode));
  n->kind = AST_TYPE_DECL;
  n->span = name.span;
  n->as.type_decl.name = (String){ .data = sdup(name.lexeme, name.lexeme_len), .len = name.lexeme_len };
  n->as.type_decl.type = type;
  return n;
}

static ASTNode *parse_oracle_decl(Parser *p) {
  Token name = expect(p, TK_IDENTIFIER, "Expected oracle name");
  expect(p, TK_COLON, "Expected :");
  ASTType *type = parse_type(p);
  expect(p, TK_EQ, "Expected =");
  Token provider = expect(p, TK_STRING, "Expected provider string");
  // skip 'via' ...
  if (token_is(peek_token(p), TK_KEYWORD) || token_is(peek_token(p), TK_IDENTIFIER)) consume_token(p);
  consume_token(p); // provider spec maybe
  expect(p, TK_SEMICOLON, "Expected ;");
  ASTNode *n = xmalloc(sizeof(ASTNode));
  n->kind = AST_ORACLE_DECL;
  n->span = name.span;
  n->as.oracle_decl.name = (String){ .data = sdup(name.lexeme, name.lexeme_len), .len = name.lexeme_len };
  n->as.oracle_decl.type = type;
  n->as.oracle_decl.provider = (String){ .data = sdup(provider.lexeme, provider.lexeme_len), .len = provider.lexeme_len };
  return n;
}

static void parse_top_level(Parser *p, ASTNode *prog) {
  const char *dbg = getenv("LIMINAL_DEBUG_PARSER"); size_t iter=0;
  for (;;) {
    if (++iter > 100000) { fprintf(stderr, "[parser] top_level iter limit\n"); break; }
    Token t = peek_token(p);
    if (dbg) fprintf(stderr, "[parser] top: %s\n", t.lexeme);
    if (t.kind == TK_EOF) break;
    if (t.kind == TK_KEYWORD && strncasecmp(t.lexeme, "begin", t.lexeme_len)==0) {
      consume_token(p);
      // body
      ASTStmtVec stmts = {0};
      while (!(peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "end", peek_token(p).lexeme_len)==0)) {
        ASTStmt *s = parse_statement(p);
        ast_stmt_vec_push(&stmts, s);
        match(p, TK_SEMICOLON);
      }
      expect(p, TK_KEYWORD, "Expected end");
      expect(p, TK_DOT, "Expected .");
      prog->as.program.body = ast_make_block(&stmts, t.span);
      break;
    }
    if (t.kind == TK_KEYWORD) {
      if (strncasecmp(t.lexeme, "uses", t.lexeme_len)==0) {
        consume_token(p);
        // simple skip identifiers until ;
        while (!match(p, TK_SEMICOLON)) consume_token(p);
        continue;
      }
      if (strncasecmp(t.lexeme, "config", t.lexeme_len)==0) {
        consume_token(p);
        while (!token_is(peek_token(p), TK_KEYWORD) || strncasecmp(peek_token(p).lexeme, "types", peek_token(p).lexeme_len)!=0) {
          Token cname = expect(p, TK_IDENTIFIER, "Expected config key");
          expect(p, TK_EQ, "Expected =");
          ASTExpr *val = parse_expression(p, 0);
          expect(p, TK_SEMICOLON, "Expected ;");
          ASTNode *n = xmalloc(sizeof(ASTNode)); n->kind = AST_CONFIG_ITEM; n->span = cname.span;
          n->as.config.name = (String){ .data = sdup(cname.lexeme, cname.lexeme_len), .len = cname.lexeme_len };
          n->as.config.value = val; n->as.config.type = NULL;
          ast_node_vec_push(&prog->as.program.config_items, n);
          if (peek_token(p).kind == TK_KEYWORD && strncasecmp(peek_token(p).lexeme, "types", peek_token(p).lexeme_len)==0) break;
        }
        continue;
      }
      if (strncasecmp(t.lexeme, "types", t.lexeme_len)==0) {
        consume_token(p);
        while (!(peek_token(p).kind == TK_KEYWORD && (strncasecmp(peek_token(p).lexeme, "oracles", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "var", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "function", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "begin", peek_token(p).lexeme_len)==0))) {
          if (peek_token(p).kind == TK_EOF) break;
          ASTNode *td = parse_type_decl(p);
          ast_node_vec_push(&prog->as.program.types, td);
        }
        continue;
      }
      if (strncasecmp(t.lexeme, "oracles", t.lexeme_len)==0) {
        consume_token(p);
        while (!(peek_token(p).kind == TK_KEYWORD && (strncasecmp(peek_token(p).lexeme, "var", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "function", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "begin", peek_token(p).lexeme_len)==0))) {
          if (peek_token(p).kind == TK_EOF) break;
          ASTNode *od = parse_oracle_decl(p);
          ast_node_vec_push(&prog->as.program.oracles, od);
        }
        continue;
      }
      if (strncasecmp(t.lexeme, "var", t.lexeme_len)==0) {
        consume_token(p);
        while (!(peek_token(p).kind == TK_KEYWORD && (strncasecmp(peek_token(p).lexeme, "function", peek_token(p).lexeme_len)==0 || strncasecmp(peek_token(p).lexeme, "begin", peek_token(p).lexeme_len)==0))) {
          if (peek_token(p).kind == TK_EOF) break;
          parse_var_decl_into(p, &prog->as.program.vars);
        }
        continue;
      }
      if (strncasecmp(t.lexeme, "function", t.lexeme_len)==0) {
        ASTNode *fn = parse_function(p);
        ast_node_vec_push(&prog->as.program.functions, fn);
        continue;
      }
    }
    consume_token(p); // skip
  }
}

static ASTNode *parse_program_internal(Parser *p) {
  Token prog = expect(p, TK_KEYWORD, "Expected program");
  if (getenv("LIMINAL_DEBUG_PARSER")) fprintf(stderr, "[parser] program %.*s\n", (int)prog.lexeme_len, prog.lexeme);
  Token name = expect(p, TK_IDENTIFIER, "Expected program name");
  expect(p, TK_SEMICOLON, "Expected ; after program name");
  ASTNode *node = ast_program_new((String){ .data = sdup(name.lexeme, name.lexeme_len), .len = name.lexeme_len });
  node->span = prog.span;
  parse_top_level(p, node);
  return node;
}

Parser *parser_create(const char *src, size_t len) {
  Parser *p = xmalloc(sizeof(Parser));
  p->lx = lexer_create(src, len);
  p->current.kind = TK_ERROR;
  p->has_lookahead = 0;
  p->errors = (ParseErrorVec){0};
  p->src = src;
  p->len = len;
  return p;
}

void parser_destroy(Parser *p) {
  for (size_t i = 0; i < p->errors.len; ++i) free(p->errors.items[i].message);
  free(p->errors.items);
  lexer_destroy(p->lx);
  free(p);
}

ASTNode *parse_program(Parser *p) { return parse_program_internal(p); }

int parser_has_errors(Parser *p) { return p->errors.len > 0; }

ParseErrorVec parser_errors(Parser *p) { return p->errors; }
