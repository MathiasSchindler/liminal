#define _POSIX_C_SOURCE 200809L
#include "liminal/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t n) {
  void *p = malloc(n);
  if (!p) { fprintf(stderr, "Out of memory\n"); exit(1); }
  memset(p, 0, n);
  return p;
}

static String str_from_token(Token tk) {
  String s;
  s.len = tk.lexeme_len;
  s.data = (char *)xmalloc(s.len + 1);
  memcpy(s.data, tk.lexeme, s.len);
  s.data[s.len] = '\0';
  return s;
}

static String string_clone(String s) {
  String c;
  c.len = s.len;
  c.data = (char *)xmalloc(c.len + 1);
  memcpy(c.data, s.data, c.len);
  c.data[c.len] = '\0';
  return c;
}

ASTNode *ast_program_new(String name) {
  ASTNode *n = (ASTNode *)xmalloc(sizeof(ASTNode));
  n->kind = AST_PROGRAM;
  n->as.program.name = name;
  return n;
}

ASTExpr *ast_make_literal(Token tk) {
  ASTExpr *e = (ASTExpr *)xmalloc(sizeof(ASTExpr));
  e->kind = EXPR_LITERAL;
  e->span = tk.span;
  e->as.literal.value = str_from_token(tk);
  e->as.literal.literal_kind = tk.kind;
  return e;
}

ASTExpr *ast_make_ident(Token tk) {
  ASTExpr *e = (ASTExpr *)xmalloc(sizeof(ASTExpr));
  e->kind = EXPR_IDENT;
  e->span = tk.span;
  e->as.ident.name = str_from_token(tk);
  return e;
}

static ASTExpr *expr_clone(const ASTExpr *e);
static ASTType *type_clone(const ASTType *t);

ASTExpr *ast_expr_clone(const ASTExpr *e) { return expr_clone(e); }
ASTType *ast_type_clone(const ASTType *t) { return type_clone(t); }

static ASTExpr *expr_clone(const ASTExpr *e) {
  if (!e) return NULL;
  ASTExpr *c = (ASTExpr *)xmalloc(sizeof(ASTExpr));
  c->kind = e->kind;
  c->span = e->span;
  switch (e->kind) {
  case EXPR_LITERAL:
    c->as.literal.value = string_clone(e->as.literal.value);
    c->as.literal.literal_kind = e->as.literal.literal_kind;
    break;
  case EXPR_IDENT:
    c->as.ident.name = string_clone(e->as.ident.name);
    break;
  case EXPR_UNARY:
    c->as.unary.op = e->as.unary.op;
    c->as.unary.expr = expr_clone(e->as.unary.expr);
    break;
  case EXPR_BINARY:
    c->as.binary.op = e->as.binary.op;
    c->as.binary.lhs = expr_clone(e->as.binary.lhs);
    c->as.binary.rhs = expr_clone(e->as.binary.rhs);
    break;
  case EXPR_CALL:
    c->as.call.callee = expr_clone(e->as.call.callee);
    c->as.call.args.len = 0; c->as.call.args.cap = 0; c->as.call.args.items = NULL;
    for (size_t i = 0; i < e->as.call.args.len; ++i) {
      ast_expr_vec_push(&c->as.call.args, expr_clone(e->as.call.args.items[i]));
    }
    break;
  case EXPR_INDEX:
    c->as.index.base = expr_clone(e->as.index.base);
    c->as.index.indices.len = 0; c->as.index.indices.cap = 0; c->as.index.indices.items = NULL;
    for (size_t i = 0; i < e->as.index.indices.len; ++i) {
      ast_expr_vec_push(&c->as.index.indices, expr_clone(e->as.index.indices.items[i]));
    }
    break;
  case EXPR_SLICE:
    c->as.slice.base = expr_clone(e->as.slice.base);
    c->as.slice.lo = expr_clone(e->as.slice.lo);
    c->as.slice.hi = expr_clone(e->as.slice.hi);
    break;
  case EXPR_FIELD:
    c->as.field.base = expr_clone(e->as.field.base);
    c->as.field.field = string_clone(e->as.field.field);
    break;
  case EXPR_TUPLE:
    c->as.tuple.elements.len = 0; c->as.tuple.elements.cap = 0; c->as.tuple.elements.items = NULL;
    for (size_t i = 0; i < e->as.tuple.elements.len; ++i) {
      ast_expr_vec_push(&c->as.tuple.elements, expr_clone(e->as.tuple.elements.items[i]));
    }
    break;
  case EXPR_ARRAY:
    c->as.array.elements.len = 0; c->as.array.elements.cap = 0; c->as.array.elements.items = NULL;
    for (size_t i = 0; i < e->as.array.elements.len; ++i) {
      ast_expr_vec_push(&c->as.array.elements, expr_clone(e->as.array.elements.items[i]));
    }
    break;
  case EXPR_RECORD:
    c->as.record.fields.len = 0; c->as.record.fields.cap = 0; c->as.record.fields.items = NULL;
    for (size_t i = 0; i < e->as.record.fields.len; ++i) {
      ASTField f = {0}; f.key = string_clone(e->as.record.fields.items[i].key);
      f.value = expr_clone(e->as.record.fields.items[i].value);
      ast_field_vec_push(&c->as.record.fields, f);
    }
    break;
  case EXPR_ASK:
    c->as.ask.oracle = expr_clone(e->as.ask.oracle);
    c->as.ask.input = expr_clone(e->as.ask.input);
    c->as.ask.into_type = type_clone(e->as.ask.into_type);
    c->as.ask.timeout = expr_clone(e->as.ask.timeout);
    c->as.ask.fallback = expr_clone(e->as.ask.fallback);
    c->as.ask.with_cost = e->as.ask.with_cost;
    break;
  case EXPR_EMBED:
    c->as.embed.oracle = expr_clone(e->as.embed.oracle);
    c->as.embed.input = expr_clone(e->as.embed.input);
    break;
  case EXPR_CONTEXT:
    c->as.context.ctx = expr_clone(e->as.context.ctx);
    c->as.context.methods.len = 0; c->as.context.methods.cap = 0; c->as.context.methods.items = NULL;
    for (size_t i = 0; i < e->as.context.methods.len; ++i) {
      ast_expr_vec_push(&c->as.context.methods, expr_clone(e->as.context.methods.items[i]));
    }
    break;
  default:
    break;
  }
  return c;
}

static ASTType *type_clone(const ASTType *t) {
  if (!t) return NULL;
  ASTType *c = (ASTType *)xmalloc(sizeof(ASTType));
  c->kind = t->kind;
  c->span = t->span;
  switch (t->kind) {
  case TYPE_PRIMITIVE:
    c->as.primitive = string_clone(t->as.primitive);
    break;
  case TYPE_IDENT:
    c->as.ident.name = string_clone(t->as.ident.name);
    break;
  case TYPE_ARRAY:
    c->as.array_type.is_fixed = t->as.array_type.is_fixed;
    c->as.array_type.elem = type_clone(t->as.array_type.elem);
    c->as.array_type.dims.len = 0; c->as.array_type.dims.cap = 0; c->as.array_type.dims.items = NULL;
    for (size_t i = 0; i < t->as.array_type.dims.len; ++i) {
      ast_expr_vec_push(&c->as.array_type.dims, expr_clone(t->as.array_type.dims.items[i]));
    }
    c->as.array_type.len_constraint = expr_clone(t->as.array_type.len_constraint);
    c->as.array_type.has_len_min = t->as.array_type.has_len_min;
    c->as.array_type.has_len_max = t->as.array_type.has_len_max;
    c->as.array_type.len_min = t->as.array_type.len_min;
    c->as.array_type.len_max = t->as.array_type.len_max;
    break;
  case TYPE_TUPLE:
    c->as.tuple_type.elements.len = 0; c->as.tuple_type.elements.cap = 0; c->as.tuple_type.elements.items = NULL;
    for (size_t i = 0; i < t->as.tuple_type.elements.len; ++i) {
      ast_type_vec_push(&c->as.tuple_type.elements, type_clone(t->as.tuple_type.elements.items[i]));
    }
    break;
  case TYPE_RECORD:
    c->as.record_type.fields.len = 0; c->as.record_type.fields.cap = 0; c->as.record_type.fields.items = NULL;
    for (size_t i = 0; i < t->as.record_type.fields.len; ++i) {
      ASTFieldDecl fd = {0};
      fd.name = string_clone(t->as.record_type.fields.items[i].name);
      fd.type = type_clone(t->as.record_type.fields.items[i].type);
      fd.describe = string_clone(t->as.record_type.fields.items[i].describe);
      ast_field_decl_vec_push(&c->as.record_type.fields, fd);
    }
    break;
  case TYPE_ENUM:
    c->as.enum_type.len = t->as.enum_type.len;
    c->as.enum_type.cap = t->as.enum_type.cap;
    c->as.enum_type.items = (String *)xmalloc(sizeof(String) * c->as.enum_type.len);
    for (size_t i = 0; i < c->as.enum_type.len; ++i) {
      c->as.enum_type.items[i] = string_clone(t->as.enum_type.items[i]);
    }
    break;
  case TYPE_OPTIONAL:
    c->as.optional_type.option = type_clone(t->as.optional_type.option);
    break;
  case TYPE_RESULT:
    c->as.result_type.ok = type_clone(t->as.result_type.ok);
    c->as.result_type.err = type_clone(t->as.result_type.err);
    break;
  case TYPE_CONSTRAINED:
    c->as.constrained_type.base = string_clone(t->as.constrained_type.base);
    c->as.constrained_type.min = t->as.constrained_type.min;
    c->as.constrained_type.max = t->as.constrained_type.max;
    c->as.constrained_type.has_min = t->as.constrained_type.has_min;
    c->as.constrained_type.has_max = t->as.constrained_type.has_max;
    c->as.constrained_type.length_constraint = t->as.constrained_type.length_constraint;
    c->as.constrained_type.pattern = t->as.constrained_type.pattern ? strdup(t->as.constrained_type.pattern) : NULL;
    break;
  case TYPE_SCHEMA:
    c->as.schema_type.name = string_clone(t->as.schema_type.name);
    c->as.schema_type.fields.len = 0; c->as.schema_type.fields.cap = 0; c->as.schema_type.fields.items = NULL;
    for (size_t i = 0; i < t->as.schema_type.fields.len; ++i) {
      ASTFieldDecl fd = {0};
      fd.name = string_clone(t->as.schema_type.fields.items[i].name);
      fd.type = type_clone(t->as.schema_type.fields.items[i].type);
      fd.describe = string_clone(t->as.schema_type.fields.items[i].describe);
      ast_field_decl_vec_push(&c->as.schema_type.fields, fd);
    }
    break;
  }
  return c;
}

void ast_node_vec_push(ASTNodeVec *vec, ASTNode *node) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTNode **)realloc(vec->items, vec->cap * sizeof(ASTNode *));
  }
  vec->items[vec->len++] = node;
}

void ast_expr_vec_push(ASTExprVec *vec, ASTExpr *expr) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTExpr **)realloc(vec->items, vec->cap * sizeof(ASTExpr *));
  }
  vec->items[vec->len++] = expr;
}

void ast_stmt_vec_push(ASTStmtVec *vec, ASTStmt *stmt) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTStmt **)realloc(vec->items, vec->cap * sizeof(ASTStmt *));
  }
  vec->items[vec->len++] = stmt;
}

void ast_type_vec_push(ASTTypeVec *vec, ASTType *type) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTType **)realloc(vec->items, vec->cap * sizeof(ASTType *));
  }
  vec->items[vec->len++] = type;
}

void ast_field_vec_push(ASTFieldVec *vec, ASTField field) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTField *)realloc(vec->items, vec->cap * sizeof(ASTField));
  }
  vec->items[vec->len++] = field;
}

void ast_param_vec_push(ASTParamVec *vec, ASTParam param) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTParam *)realloc(vec->items, vec->cap * sizeof(ASTParam));
  }
  vec->items[vec->len++] = param;
}

void ast_var_decl_vec_push(ASTVarDeclVec *vec, ASTVarDecl decl) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTVarDecl *)realloc(vec->items, vec->cap * sizeof(ASTVarDecl));
  }
  vec->items[vec->len++] = decl;
}

void ast_field_decl_vec_push(ASTFieldDeclVec *vec, ASTFieldDecl decl) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = (ASTFieldDecl *)realloc(vec->items, vec->cap * sizeof(ASTFieldDecl));
  }
  vec->items[vec->len++] = decl;
}
void ast_enum_vec_push(ASTEnumType *vec, String item) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = realloc(vec->items, vec->cap * sizeof(String));
  }
  vec->items[vec->len++] = item;
}
ASTStmt *ast_make_block(ASTStmtVec *stmts, LiminalSpan span) {
  ASTStmt *s = (ASTStmt *)xmalloc(sizeof(ASTStmt));
  s->kind = STMT_BLOCK;
  s->span = span;
  s->as.block.stmts = *stmts;
  return s;
}

static void free_string(String *s) { if (s && s->data) free(s->data); }

static void free_expr(ASTExpr *e);
static void free_type(ASTType *t);
void ast_type_free(ASTType *t) { free_type(t); }
static void free_stmt(ASTStmt *s);
static void free_node(ASTNode *n);

static void free_expr(ASTExpr *e) {
  if (!e) return;
  switch (e->kind) {
  case EXPR_LITERAL:
    free_string(&e->as.literal.value);
    break;
  case EXPR_IDENT:
    free_string(&e->as.ident.name);
    break;
  case EXPR_UNARY:
    free_expr(e->as.unary.expr);
    break;
  case EXPR_BINARY:
    free_expr(e->as.binary.lhs);
    free_expr(e->as.binary.rhs);
    break;
  case EXPR_CALL:
    free_expr(e->as.call.callee);
    for (size_t i = 0; i < e->as.call.args.len; ++i) free_expr(e->as.call.args.items[i]);
    free(e->as.call.args.items);
    break;
  case EXPR_INDEX:
    free_expr(e->as.index.base);
    for (size_t i = 0; i < e->as.index.indices.len; ++i) free_expr(e->as.index.indices.items[i]);
    free(e->as.index.indices.items);
    break;
  case EXPR_SLICE:
    free_expr(e->as.slice.base);
    free_expr(e->as.slice.lo);
    free_expr(e->as.slice.hi);
    break;
  case EXPR_FIELD:
    free_expr(e->as.field.base);
    free_string(&e->as.field.field);
    break;
  case EXPR_TUPLE:
  case EXPR_ARRAY:
    for (size_t i = 0; i < e->as.tuple.elements.len; ++i) free_expr(e->as.tuple.elements.items[i]);
    free(e->as.tuple.elements.items);
    break;
  case EXPR_RECORD:
    for (size_t i = 0; i < e->as.record.fields.len; ++i) {
      free_string(&e->as.record.fields.items[i].key);
      free_expr(e->as.record.fields.items[i].value);
    }
    free(e->as.record.fields.items);
    break;
  case EXPR_ASK:
    free_expr(e->as.ask.oracle);
    free_expr(e->as.ask.input);
    free_type(e->as.ask.into_type);
    free_expr(e->as.ask.timeout);
    free_expr(e->as.ask.fallback);
    break;
  case EXPR_CONSULT:
    free_expr(e->as.consult.oracle);
    free_expr(e->as.consult.input);
    free_type(e->as.consult.into_type);
    free_expr(e->as.consult.hint);
    free_expr(e->as.consult.fallback);
    break;
  case EXPR_CONCAT:
    free_expr(e->as.concat.lhs);
    free_expr(e->as.concat.rhs);
    break;
  case EXPR_EMBED:
    free_expr(e->as.embed.oracle);
    free_expr(e->as.embed.input);
    break;
  case EXPR_CONTEXT:
    free_expr(e->as.context.ctx);
    for (size_t i = 0; i < e->as.context.methods.len; ++i) free_expr(e->as.context.methods.items[i]);
    free(e->as.context.methods.items);
    break;
  }
  free(e);
}

static void free_type(ASTType *t) {
  if (!t) return;
  switch (t->kind) {
  case TYPE_PRIMITIVE:
    free_string(&t->as.primitive);
    break;
  case TYPE_IDENT:
    free_string(&t->as.ident.name);
    break;
  case TYPE_ARRAY:
    free_type(t->as.array_type.elem);
    for (size_t i = 0; i < t->as.array_type.dims.len; ++i) free_expr(t->as.array_type.dims.items[i]);
    free(t->as.array_type.dims.items);
    free_expr(t->as.array_type.len_constraint);
    break;
  case TYPE_TUPLE:
    for (size_t i = 0; i < t->as.tuple_type.elements.len; ++i) free_type(t->as.tuple_type.elements.items[i]);
    free(t->as.tuple_type.elements.items);
    break;
  case TYPE_RECORD:
    for (size_t i = 0; i < t->as.record_type.fields.len; ++i) {
      free_string(&t->as.record_type.fields.items[i].name);
      free_type(t->as.record_type.fields.items[i].type);
    }
    free(t->as.record_type.fields.items);
    break;
  case TYPE_ENUM:
    for (size_t i = 0; i < t->as.enum_type.len; ++i) free_string(&t->as.enum_type.items[i]);
    free(t->as.enum_type.items);
    break;
  case TYPE_OPTIONAL:
    free_type(t->as.optional_type.option);
    break;
  case TYPE_RESULT:
    free_type(t->as.result_type.ok);
    free_type(t->as.result_type.err);
    break;
  case TYPE_CONSTRAINED:
    free_string(&t->as.constrained_type.base);
    free(t->as.constrained_type.pattern);
    break;
  case TYPE_SCHEMA:
    free_string(&t->as.schema_type.name);
    for (size_t i = 0; i < t->as.schema_type.fields.len; ++i) {
      free_string(&t->as.schema_type.fields.items[i].name);
      free_type(t->as.schema_type.fields.items[i].type);
      free_string(&t->as.schema_type.fields.items[i].describe);
    }
    free(t->as.schema_type.fields.items);
    break;
  }
  free(t);
}

static void free_stmt(ASTStmt *s) {
  if (!s) return;
  switch (s->kind) {
  case STMT_ASSIGN:
    free_expr(s->as.assign.target);
    free_expr(s->as.assign.value);
    break;
  case STMT_IF:
    free_expr(s->as.if_stmt.cond);
    free_stmt(s->as.if_stmt.then_branch);
    free_stmt(s->as.if_stmt.else_branch);
    break;
  case STMT_WHILE:
    free_expr(s->as.while_stmt.cond);
    free_stmt(s->as.while_stmt.body);
    break;
  case STMT_FOR:
    free_expr(s->as.for_stmt.init);
    free_expr(s->as.for_stmt.to);
    free_stmt(s->as.for_stmt.body);
    free_string(&s->as.for_stmt.var.name);
    break;
  case STMT_FOR_IN:
    free_expr(s->as.for_in_stmt.iterable);
    free_stmt(s->as.for_in_stmt.body);
    free_string(&s->as.for_in_stmt.var.name);
    break;
  case STMT_REPEAT:
    free_stmt(s->as.repeat_stmt.body);
    free_expr(s->as.repeat_stmt.cond);
    break;
  case STMT_CASE:
    free_expr(s->as.case_stmt.expr);
    for (size_t i = 0; i < s->as.case_stmt.patterns.len; ++i) free_expr(s->as.case_stmt.patterns.items[i]);
    for (size_t i = 0; i < s->as.case_stmt.branches.len; ++i) free_stmt(s->as.case_stmt.branches.items[i]);
    free(s->as.case_stmt.patterns.items);
    free(s->as.case_stmt.branches.items);
    free_stmt(s->as.case_stmt.else_branch);
    break;
  case STMT_LOOP:
    for (size_t i = 0; i < s->as.loop_stmt.body.len; ++i) free_stmt(s->as.loop_stmt.body.items[i]);
    free(s->as.loop_stmt.body.items);
    break;
  case STMT_PARALLEL:
    for (size_t i = 0; i < s->as.parallel_stmt.body.len; ++i) free_stmt(s->as.parallel_stmt.body.items[i]);
    free(s->as.parallel_stmt.body.items);
    break;
  case STMT_BREAK:
  case STMT_CONTINUE:
    break;
  case STMT_RETURN:
    free_expr(s->as.return_stmt.value);
    break;
  case STMT_TRY:
    free_stmt(s->as.try_stmt.try_block);
    free_stmt(s->as.try_stmt.except_block);
    break;
  case STMT_BLOCK:
    for (size_t i = 0; i < s->as.block.stmts.len; ++i) free_stmt(s->as.block.stmts.items[i]);
    free(s->as.block.stmts.items);
    break;
  case STMT_EXPR:
    free_expr(s->as.expr_stmt.expr);
    break;
  }
  free(s);
}

static void free_node(ASTNode *n) {
  if (!n) return;
  switch (n->kind) {
  case AST_PROGRAM:
    free_string(&n->as.program.name);
    for (size_t i = 0; i < n->as.program.uses.len; ++i) free_node(n->as.program.uses.items[i]);
    for (size_t i = 0; i < n->as.program.config_items.len; ++i) free_node(n->as.program.config_items.items[i]);
    for (size_t i = 0; i < n->as.program.types.len; ++i) free_node(n->as.program.types.items[i]);
    for (size_t i = 0; i < n->as.program.oracles.len; ++i) free_node(n->as.program.oracles.items[i]);
    for (size_t i = 0; i < n->as.program.vars.len; ++i) free_node(n->as.program.vars.items[i]);
    for (size_t i = 0; i < n->as.program.functions.len; ++i) free_node(n->as.program.functions.items[i]);
    free(n->as.program.uses.items);
    free(n->as.program.config_items.items);
    free(n->as.program.types.items);
    free(n->as.program.oracles.items);
    free(n->as.program.vars.items);
    free(n->as.program.functions.items);
    free_stmt(n->as.program.body);
    break;
  case AST_CONFIG_ITEM:
    free_string(&n->as.config.name);
    free_type(n->as.config.type);
    free_expr(n->as.config.value);
    break;
  case AST_TYPE_DECL:
    free_string(&n->as.type_decl.name);
    free_type(n->as.type_decl.type);
    break;
  case AST_ORACLE_DECL:
    free_string(&n->as.oracle_decl.name);
    free_type(n->as.oracle_decl.type);
    free_string(&n->as.oracle_decl.provider);
    break;
  case AST_VAR_DECL:
    free_string(&n->as.var_decl.name);
    free_type(n->as.var_decl.type);
    free_expr(n->as.var_decl.init);
    break;
  case AST_FUNC_DECL:
    free_string(&n->as.func_decl.name);
    for (size_t i = 0; i < n->as.func_decl.params.len; ++i) {
      free_string(&n->as.func_decl.params.items[i].name);
      free_type(n->as.func_decl.params.items[i].type);
    }
    free(n->as.func_decl.params.items);
    free_type(n->as.func_decl.result_type);
    if (n->as.func_decl.locals) {
      for (size_t i = 0; i < n->as.func_decl.locals->vars.len; ++i) {
        free_string(&n->as.func_decl.locals->vars.items[i].name);
        free_type(n->as.func_decl.locals->vars.items[i].type);
        free_expr(n->as.func_decl.locals->vars.items[i].init);
      }
      free(n->as.func_decl.locals->vars.items);
      free(n->as.func_decl.locals);
    }
    free_stmt(n->as.func_decl.body);
    break;
  default:
    break;
  }
  free(n);
}

void ast_free(ASTNode *node) { free_node(node); }

static void indent(FILE *out, int level) {
  for (int i = 0; i < level; ++i) fputs("  ", out);
}

static void print_expr(const ASTExpr *e, FILE *out, int level);
static void print_type(const ASTType *t, FILE *out, int level);
static void print_stmt(const ASTStmt *s, FILE *out, int level);


static void print_expr(const ASTExpr *e, FILE *out, int level) {
  if (!e) { indent(out, level); fputs("<null expr>\n", out); return; }
  indent(out, level);
  switch (e->kind) {
  case EXPR_LITERAL:
    fprintf(out, "Literal(%s)\n", e->as.literal.value.data);
    break;
  case EXPR_IDENT:
    fprintf(out, "Ident(%s)\n", e->as.ident.name.data);
    break;
  case EXPR_UNARY:
    fprintf(out, "Unary(%d)\n", e->as.unary.op);
    print_expr(e->as.unary.expr, out, level + 1);
    break;
  case EXPR_BINARY:
    fprintf(out, "Binary(%d)\n", e->as.binary.op);
    print_expr(e->as.binary.lhs, out, level + 1);
    print_expr(e->as.binary.rhs, out, level + 1);
    break;
  case EXPR_CALL:
    fputs("Call\n", out);
    print_expr(e->as.call.callee, out, level + 1);
    for (size_t i = 0; i < e->as.call.args.len; ++i) print_expr(e->as.call.args.items[i], out, level + 1);
    break;
  case EXPR_INDEX:
    fputs("Index\n", out);
    print_expr(e->as.index.base, out, level + 1);
    for (size_t i = 0; i < e->as.index.indices.len; ++i) print_expr(e->as.index.indices.items[i], out, level + 1);
    break;
  case EXPR_SLICE:
    fputs("Slice\n", out);
    print_expr(e->as.slice.base, out, level + 1);
    print_expr(e->as.slice.lo, out, level + 1);
    print_expr(e->as.slice.hi, out, level + 1);
    break;
  case EXPR_FIELD:
    fprintf(out, "Field(%s)\n", e->as.field.field.data);
    print_expr(e->as.field.base, out, level + 1);
    break;
  case EXPR_TUPLE:
    fputs("Tuple\n", out);
    for (size_t i = 0; i < e->as.tuple.elements.len; ++i) print_expr(e->as.tuple.elements.items[i], out, level + 1);
    break;
  case EXPR_ARRAY:
    fputs("Array\n", out);
    for (size_t i = 0; i < e->as.array.elements.len; ++i) print_expr(e->as.array.elements.items[i], out, level + 1);
    break;
  case EXPR_RECORD:
    fputs("Record\n", out);
    for (size_t i = 0; i < e->as.record.fields.len; ++i) {
      indent(out, level + 1);
      fprintf(out, "Field %s:\n", e->as.record.fields.items[i].key.data);
      print_expr(e->as.record.fields.items[i].value, out, level + 2);
    }
    break;
  case EXPR_ASK:
    fputs("Ask\n", out);
    print_expr(e->as.ask.oracle, out, level + 1);
    print_expr(e->as.ask.input, out, level + 1);
    if (e->as.ask.into_type) { indent(out, level + 1); fputs("Into:\n", out); print_type(e->as.ask.into_type, out, level + 2); }
    if (e->as.ask.timeout) { indent(out, level + 1); fputs("Timeout:\n", out); print_expr(e->as.ask.timeout, out, level + 2); }
    if (e->as.ask.fallback) { indent(out, level + 1); fputs("Else:\n", out); print_expr(e->as.ask.fallback, out, level + 2); }
    break;
  case EXPR_CONSULT:
    fputs("Consult\n", out);
    print_expr(e->as.consult.oracle, out, level + 1);
    print_expr(e->as.consult.input, out, level + 1);
    if (e->as.consult.into_type) { indent(out, level + 1); fputs("Into:\n", out); print_type(e->as.consult.into_type, out, level + 2); }
    indent(out, level + 1); fprintf(out, "Attempts: %d\n", e->as.consult.attempts);
    if (e->as.consult.hint) { indent(out, level + 1); fputs("Hint:\n", out); print_expr(e->as.consult.hint, out, level + 2); }
    if (e->as.consult.fallback) { indent(out, level + 1); fputs("Yield:\n", out); print_expr(e->as.consult.fallback, out, level + 2); }
    break;
  case EXPR_CONCAT:
    fputs("Concat\n", out);
    print_expr(e->as.concat.lhs, out, level + 1);
    print_expr(e->as.concat.rhs, out, level + 1);
    break;
  case EXPR_EMBED:
    fputs("Embed\n", out);
    print_expr(e->as.embed.oracle, out, level + 1);
    print_expr(e->as.embed.input, out, level + 1);
    break;
  case EXPR_CONTEXT:
    fputs("Context\n", out);
    print_expr(e->as.context.ctx, out, level + 1);
    for (size_t i = 0; i < e->as.context.methods.len; ++i) print_expr(e->as.context.methods.items[i], out, level + 1);
    break;
  }
}

static void print_type(const ASTType *t, FILE *out, int level) {
  if (!t) { indent(out, level); fputs("<null type>\n", out); return; }
  indent(out, level);
  switch (t->kind) {
  case TYPE_PRIMITIVE:
    fprintf(out, "Primitive(%s)\n", t->as.primitive.data);
    break;
  case TYPE_IDENT:
    fprintf(out, "Ident(%s)\n", t->as.ident.name.data);
    break;
  case TYPE_ARRAY:
    fputs("Array\n", out);
    print_type(t->as.array_type.elem, out, level + 1);
    break;
  case TYPE_TUPLE:
    fputs("TupleType\n", out);
    for (size_t i = 0; i < t->as.tuple_type.elements.len; ++i) print_type(t->as.tuple_type.elements.items[i], out, level + 1);
    break;
  case TYPE_RECORD:
    fputs("RecordType\n", out);
    for (size_t i = 0; i < t->as.record_type.fields.len; ++i) {
      indent(out, level + 1);
      fprintf(out, "Field %s:\n", t->as.record_type.fields.items[i].name.data);
      print_type(t->as.record_type.fields.items[i].type, out, level + 2);
    }
    break;
  case TYPE_ENUM:
    fputs("Enum\n", out);
    break;
  case TYPE_OPTIONAL:
    fputs("Optional\n", out);
    print_type(t->as.optional_type.option, out, level + 1);
    break;
  case TYPE_RESULT:
    fputs("Result\n", out);
    print_type(t->as.result_type.ok, out, level + 1);
    if (t->as.result_type.err) print_type(t->as.result_type.err, out, level + 1);
    break;
  case TYPE_CONSTRAINED:
    fprintf(out, "Constrained(%s)\n", t->as.constrained_type.base.data);
    break;
  case TYPE_SCHEMA:
    fprintf(out, "Schema(%s)\n", t->as.schema_type.name.data);
    for (size_t i = 0; i < t->as.schema_type.fields.len; ++i) {
      indent(out, level + 1);
      fprintf(out, "Field %s:\n", t->as.schema_type.fields.items[i].name.data);
      print_type(t->as.schema_type.fields.items[i].type, out, level + 2);
    }
    break;
  }
}

static void print_stmt(const ASTStmt *s, FILE *out, int level) {
  if (!s) { indent(out, level); fputs("<null stmt>\n", out); return; }
  indent(out, level);
  switch (s->kind) {
  case STMT_ASSIGN:
    fputs("Assign\n", out);
    print_expr(s->as.assign.target, out, level + 1);
    print_expr(s->as.assign.value, out, level + 1);
    break;
  case STMT_IF:
    fputs("If\n", out);
    print_expr(s->as.if_stmt.cond, out, level + 1);
    print_stmt(s->as.if_stmt.then_branch, out, level + 1);
    print_stmt(s->as.if_stmt.else_branch, out, level + 1);
    break;
  case STMT_WHILE:
    fputs("While\n", out);
    print_expr(s->as.while_stmt.cond, out, level + 1);
    print_stmt(s->as.while_stmt.body, out, level + 1);
    break;
  case STMT_FOR:
    fputs("For\n", out);
    print_expr(s->as.for_stmt.init, out, level + 1);
    print_expr(s->as.for_stmt.to, out, level + 1);
    print_stmt(s->as.for_stmt.body, out, level + 1);
    break;
  case STMT_FOR_IN:
    fputs("ForIn\n", out);
    print_expr(s->as.for_in_stmt.iterable, out, level + 1);
    print_stmt(s->as.for_in_stmt.body, out, level + 1);
    break;
  case STMT_REPEAT:
    fputs("Repeat\n", out);
    print_stmt(s->as.repeat_stmt.body, out, level + 1);
    print_expr(s->as.repeat_stmt.cond, out, level + 1);
    break;
  case STMT_CASE:
    fputs("Case\n", out);
    print_expr(s->as.case_stmt.expr, out, level + 1);
    for (size_t i = 0; i < s->as.case_stmt.patterns.len; ++i) {
      indent(out, level + 1);
      fputs("Pattern:\n", out);
      print_expr(s->as.case_stmt.patterns.items[i], out, level + 2);
      print_stmt(s->as.case_stmt.branches.items[i], out, level + 1);
    }
    if (s->as.case_stmt.else_branch) {
      indent(out, level + 1);
      fputs("Else:\n", out);
      print_stmt(s->as.case_stmt.else_branch, out, level + 2);
    }
    break;
  case STMT_LOOP:
    fputs("Loop\n", out);
    for (size_t i = 0; i < s->as.loop_stmt.body.len; ++i) print_stmt(s->as.loop_stmt.body.items[i], out, level + 1);
    break;
  case STMT_PARALLEL:
    fputs("Parallel\n", out);
    for (size_t i = 0; i < s->as.parallel_stmt.body.len; ++i) print_stmt(s->as.parallel_stmt.body.items[i], out, level + 1);
    break;
  case STMT_BREAK:
    fputs("Break\n", out);
    break;
  case STMT_CONTINUE:
    fputs("Continue\n", out);
    break;
  case STMT_RETURN:
    fputs("Return\n", out);
    print_expr(s->as.return_stmt.value, out, level + 1);
    break;
  case STMT_TRY:
    fputs("Try\n", out);
    print_stmt(s->as.try_stmt.try_block, out, level + 1);
    print_stmt(s->as.try_stmt.except_block, out, level + 1);
    break;
  case STMT_BLOCK:
    fputs("Block\n", out);
    for (size_t i = 0; i < s->as.block.stmts.len; ++i) print_stmt(s->as.block.stmts.items[i], out, level + 1);
    break;
  case STMT_EXPR:
    fputs("ExprStmt\n", out);
    print_expr(s->as.expr_stmt.expr, out, level + 1);
    break;
  }
}

void ast_print(const ASTNode *node, FILE *out) {
  if (!node) { fputs("<null node>\n", out); return; }
  switch (node->kind) {
  case AST_PROGRAM:
    fprintf(out, "Program %s\n", node->as.program.name.data);
    if (node->as.program.uses.len) { fputs("Uses:\n", out); /* no uses nodes yet */ }
    if (node->as.program.config_items.len) { fputs("Config:\n", out); }
    if (node->as.program.types.len) { fputs("Types:\n", out); }
    if (node->as.program.oracles.len) { fputs("Oracles:\n", out); }
    if (node->as.program.vars.len) { fputs("Vars:\n", out); }
    if (node->as.program.functions.len) { fputs("Functions:\n", out); }
    if (node->as.program.body) { fputs("Body:\n", out); print_stmt(node->as.program.body, out, 1); }
    break;
  default:
    fputs("<unknown node>\n", out);
    break;
  }
}
