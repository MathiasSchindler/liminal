#define _POSIX_C_SOURCE 200809L
#include "liminal/ir.h"
#include "liminal/symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} memset(p,0,n); return p; }

static const char *op_name(IrOp op) {
  switch (op) {
  case IR_NOP: return "NOP";
  case IR_CONST_INT: return "CONST_INT";
  case IR_CONST_REAL: return "CONST_REAL";
  case IR_CONST_STRING: return "CONST_STRING";
  case IR_LOAD_VAR: return "LOAD_VAR";
  case IR_STORE_VAR: return "STORE_VAR";
  case IR_ADD: return "ADD";
  case IR_SUB: return "SUB";
  case IR_MUL: return "MUL";
  case IR_DIV: return "DIV";
  case IR_MOD: return "MOD";
  case IR_EQ: return "EQ";
  case IR_NEQ: return "NEQ";
  case IR_LT: return "LT";
  case IR_GT: return "GT";
  case IR_LE: return "LE";
  case IR_GE: return "GE";
  case IR_JUMP: return "JUMP";
  case IR_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
  case IR_LABEL: return "LABEL";
  case IR_RET: return "RET";
  case IR_PRINT: return "PRINT";
  case IR_PRINTLN: return "PRINTLN";
  case IR_READLN: return "READLN";
  case IR_READ_FILE: return "READ_FILE";
  case IR_WRITE_FILE: return "WRITE_FILE";
  case IR_ASK: return "ASK";
  case IR_RESULT_UNWRAP: return "RESULT_UNWRAP";
  case IR_RESULT_IS_OK: return "RESULT_IS_OK";
  case IR_RESULT_UNWRAP_ERR: return "RESULT_UNWRAP_ERR";
  case IR_MAKE_RESULT_OK: return "RESULT_OK";
  case IR_MAKE_RESULT_ERR: return "RESULT_ERR";
  case IR_CONCAT: return "CONCAT";
  case IR_RESULT_OR_FALLBACK: return "RESULT_OR_FALLBACK";
  case IR_CALL: return "CALL";
  case IR_AND: return "AND";
  case IR_OR: return "OR";
  case IR_CONST_BOOL: return "CONST_BOOL";
  case IR_CONST_OPTIONAL_NONE: return "CONST_OPTIONAL_NONE";
  case IR_INDEX: return "INDEX";
  }
  return "?";
}

IrProgram *ir_program_new(void) {
  IrProgram *p = xmalloc(sizeof(IrProgram));
  memset(p, 0, sizeof(IrProgram));
  return p;
}

static void ir_instr_vec_push(IrInstrVec *v, IrInstr instr) {
  if (v->len == v->cap) {
    v->cap = v->cap ? v->cap * 2 : 8;
    v->items = realloc(v->items, v->cap * sizeof(IrInstr));
  }
  v->items[v->len++] = instr;
}

static void ir_func_vec_push(IrFuncVec *v, IrFunc f) {
  if (v->len == v->cap) {
    v->cap = v->cap ? v->cap * 2 : 4;
    v->items = realloc(v->items, v->cap * sizeof(IrFunc));
  }
  v->items[v->len++] = f;
}

void ir_program_add_func(IrProgram *prog, IrFunc func) {
  ir_func_vec_push(&prog->funcs, func);
}

static void free_instrs(IrInstrVec *v) {
  for (size_t i = 0; i < v->len; ++i) {
    free(v->items[i].s);
    free(v->items[i].s2);
  }
  free(v->items);
}

void ir_program_free(IrProgram *prog) {
  if (!prog) return;
  for (size_t i = 0; i < prog->funcs.len; ++i) {
    free(prog->funcs.items[i].name);
    for (int j = 0; j < prog->funcs.items[i].param_count; ++j) free(prog->funcs.items[i].params[j]);
    free(prog->funcs.items[i].params);
    free_instrs(&prog->funcs.items[i].instrs);
  }
  free(prog->funcs.items);
  for (size_t i = 0; i < prog->schemas.len; ++i) type_free(prog->schemas.items[i]);
  free(prog->schemas.items);
  free(prog);
}

char *ir_program_print(const IrProgram *prog) {
  size_t cap = 1024, len = 0;
  char *buf = xmalloc(cap);
  buf[0] = '\0';
  if (prog->schemas.len > 0) {
    int n = snprintf(buf + len, cap - len, "schemas\n");
    if (len + n + 1 > cap) { cap *= 2; buf = realloc(buf, cap); n = snprintf(buf + len, cap - len, "schemas\n"); }
    len += n;
    for (size_t si = 0; si < prog->schemas.len; ++si) {
      Type *s = prog->schemas.items[si];
      n = snprintf(buf + len, cap - len, "  %s\n", s->as.schema.name ? s->as.schema.name : "(null)");
      if (len + n + 1 > cap) { cap *= 2; buf = realloc(buf, cap); n = snprintf(buf + len, cap - len, "  %s\n", s->as.schema.name ? s->as.schema.name : "(null)"); }
      len += n;
      for (size_t fi = 0; fi < s->as.schema.len; ++fi) {
        SchemaField *sf = &s->as.schema.items[fi];
        n = snprintf(buf + len, cap - len, "    %s\n", sf->name ? sf->name : "(null)");
        if (len + n + 1 > cap) { cap *= 2; buf = realloc(buf, cap); n = snprintf(buf + len, cap - len, "    %s\n", sf->name ? sf->name : "(null)"); }
        len += n;
      }
    }
  }
  for (size_t i = 0; i < prog->funcs.len; ++i) {
    const IrFunc *f = &prog->funcs.items[i];
    int n = snprintf(buf + len, cap - len, "func %s\n", f->name);
    if (len + n + 1 > cap) { cap *= 2; buf = realloc(buf, cap); i--; continue; }
    len += n;
    for (size_t j = 0; j < f->instrs.len; ++j) {
      const IrInstr *ins = &f->instrs.items[j];
      switch (ins->op) {
      case IR_CONST_INT:
        n = snprintf(buf + len, cap - len, "  t%d = %s %d\n", ins->dest, op_name(ins->op), ins->arg1);
        break;
      case IR_CONST_REAL:
        n = snprintf(buf + len, cap - len, "  t%d = %s %g\n", ins->dest, op_name(ins->op), ins->f);
        break;
      case IR_CONST_STRING:
        n = snprintf(buf + len, cap - len, "  t%d = %s \"%s\"\n", ins->dest, op_name(ins->op), ins->s ? ins->s : "");
        break;
      case IR_CONST_OPTIONAL_NONE:
        n = snprintf(buf + len, cap - len, "  t%d = CONST_NONE\n", ins->dest);
        break;
    case IR_CONST_BOOL:
        n = snprintf(buf + len, cap - len, "  t%d = %s %d\n", ins->dest, op_name(ins->op), ins->arg1);
        break;
    case IR_INDEX:
        n = snprintf(buf + len, cap - len, "  t%d = %s %s t%d\n", ins->dest, op_name(ins->op), ins->s?ins->s:"", ins->arg2);
        break;
      case IR_LOAD_VAR:
        n = snprintf(buf + len, cap - len, "  t%d = %s %s\n", ins->dest, op_name(ins->op), ins->s);
        break;
      case IR_STORE_VAR:
        n = snprintf(buf + len, cap - len, "  %s = t%d\n", ins->s, ins->arg1);
        break;
      case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
      case IR_EQ: case IR_NEQ: case IR_LT: case IR_GT: case IR_LE: case IR_GE:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_JUMP:
        n = snprintf(buf + len, cap - len, "  %s L%s\n", op_name(ins->op), ins->s);
        break;
      case IR_JUMP_IF_FALSE:
        n = snprintf(buf + len, cap - len, "  %s t%d, L%s\n", op_name(ins->op), ins->arg1, ins->s);
        break;
      case IR_LABEL:
        n = snprintf(buf + len, cap - len, "L%s:\n", ins->s);
        break;
      case IR_RET:
        n = snprintf(buf + len, cap - len, "  RET t%d\n", ins->arg1);
        break;
      case IR_PRINT:
        n = snprintf(buf + len, cap - len, "  %s t%d\n", op_name(ins->op), ins->arg1);
        break;
      case IR_PRINTLN:
        if (ins->arg1 >= 0) n = snprintf(buf + len, cap - len, "  %s t%d\n", op_name(ins->op), ins->arg1);
        else n = snprintf(buf + len, cap - len, "  %s\n", op_name(ins->op));
        break;
      case IR_READLN:
        n = snprintf(buf + len, cap - len, "  %s %s\n", op_name(ins->op), ins->s);
        break;
      case IR_READ_FILE:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d\n", ins->dest, op_name(ins->op), ins->arg1);
        break;
      case IR_WRITE_FILE:
        n = snprintf(buf + len, cap - len, "  %s t%d, t%d\n", op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_ASK:
        if (ins->s2 && ins->s2[0])
          n = snprintf(buf + len, cap - len, "  t%d = %s t%d, fallback t%d oracle %s schema %s\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2, ins->s ? ins->s : "", ins->s2);
        else
          n = snprintf(buf + len, cap - len, "  t%d = %s t%d, fallback t%d oracle %s\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2, ins->s ? ins->s : "");
        break;
      case IR_RESULT_UNWRAP:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_RESULT_UNWRAP_ERR:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d\n", ins->dest, op_name(ins->op), ins->arg1);
        break;
      case IR_RESULT_IS_OK:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d\n", ins->dest, op_name(ins->op), ins->arg1);
        break;
      case IR_CONCAT:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_RESULT_OR_FALLBACK:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_AND:
      case IR_OR:
        n = snprintf(buf + len, cap - len, "  t%d = %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->arg1, ins->arg2);
        break;
      case IR_CALL:
        if (ins->arg2 >= 0)
          n = snprintf(buf + len, cap - len, "  t%d = %s %s t%d, t%d\n", ins->dest, op_name(ins->op), ins->s ? ins->s : "", ins->arg1, ins->arg2);
        else
          n = snprintf(buf + len, cap - len, "  t%d = %s %s t%d\n", ins->dest, op_name(ins->op), ins->s ? ins->s : "", ins->arg1);
        break;
      case IR_NOP:
      default:
        n = snprintf(buf + len, cap - len, "  %s\n", op_name(ins->op));
      }
      if (len + n + 1 > cap) { cap *= 2; buf = realloc(buf, cap); j--; continue; }
      len += n;
    }
    if (len + 2 > cap) { cap *= 2; buf = realloc(buf, cap); }
    buf[len++] = '\n'; buf[len] = '\0';
  }
  return buf;
}

// Builder helpers
static int ir_func_new_temp(IrFunc *f) { return f->next_temp++; }

static void emit(IrInstrVec *v, IrInstr ins) { ir_instr_vec_push(v, ins); }

// Public builder API (used by translator)
IrFunc ir_func_create(const char *name) {
  IrFunc f = {0};
  f.name = strdup(name);
  f.params = NULL;
  f.param_count = 0;
  f.next_temp = 0;
  return f;
}

int ir_emit_const_int(IrFunc *f, int value) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONST_INT, .dest = t, .arg1 = value};
  emit(&f->instrs, ins);
  return t;
}
int ir_emit_const_bool(IrFunc *f, int value) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONST_BOOL, .dest = t, .arg1 = value};
  emit(&f->instrs, ins);
  return t;
}
int ir_emit_const_optional_none(IrFunc *f) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONST_OPTIONAL_NONE, .dest = t};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_const_string(IrFunc *f, const char *s) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONST_STRING, .dest = t, .s = strdup(s)};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_const_real(IrFunc *f, double value) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONST_REAL, .dest = t, .f = value};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_binop(IrFunc *f, IrOp op, int lhs, int rhs) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = op, .dest = t, .arg1 = lhs, .arg2 = rhs};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_load_var(IrFunc *f, const char *name) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_LOAD_VAR, .dest = t, .s = strdup(name)};
  emit(&f->instrs, ins);
  return t;
}

void ir_emit_store_var(IrFunc *f, const char *name, int src_temp) {
  IrInstr ins = {.op = IR_STORE_VAR, .s = strdup(name), .arg1 = src_temp};
  emit(&f->instrs, ins);
}

void ir_emit_jump(IrFunc *f, const char *label) {
  IrInstr ins = {.op = IR_JUMP, .s = strdup(label)};
  emit(&f->instrs, ins);
}

void ir_emit_jump_if_false(IrFunc *f, int cond_temp, const char *label) {
  IrInstr ins = {.op = IR_JUMP_IF_FALSE, .arg1 = cond_temp, .s = strdup(label)};
  emit(&f->instrs, ins);
}

void ir_emit_label(IrFunc *f, const char *label) {
  IrInstr ins = {.op = IR_LABEL, .s = strdup(label)};
  emit(&f->instrs, ins);
}

void ir_emit_ret(IrFunc *f, int temp) {
  IrInstr ins = {.op = IR_RET, .arg1 = temp};
  emit(&f->instrs, ins);
}

void ir_emit_print(IrFunc *f, int temp, int newline) {
  IrInstr ins = {.op = newline ? IR_PRINTLN : IR_PRINT, .arg1 = temp};
  emit(&f->instrs, ins);
}

void ir_emit_readln(IrFunc *f, const char *name) {
  IrInstr ins = {.op = IR_READLN, .s = strdup(name)};
  emit(&f->instrs, ins);
}

int ir_emit_read_file(IrFunc *f, int path_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_READ_FILE, .dest = t, .arg1 = path_temp};
  emit(&f->instrs, ins);
  return t;
}

void ir_emit_write_file(IrFunc *f, int path_temp, int content_temp) {
  IrInstr ins = {.op = IR_WRITE_FILE, .arg1 = path_temp, .arg2 = content_temp};
  emit(&f->instrs, ins);
}

int ir_emit_ask(IrFunc *f, int prompt_temp, int fallback_temp, const char *oracle_name, const char *schema_name) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_ASK, .dest = t, .arg1 = prompt_temp, .arg2 = fallback_temp,
                 .s = oracle_name ? strdup(oracle_name) : NULL,
                 .s2 = schema_name ? strdup(schema_name) : NULL};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_result_unwrap_err(IrFunc *f, int result_temp) {
  IrInstr ins = {.op = IR_RESULT_UNWRAP_ERR, .dest = ir_func_new_temp(f), .arg1 = result_temp};
  emit(&f->instrs, ins);
  return ins.dest;
}

int ir_emit_result_unwrap(IrFunc *f, int result_temp, int fallback_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_RESULT_UNWRAP, .dest = t, .arg1 = result_temp, .arg2 = fallback_temp};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_result_is_ok(IrFunc *f, int result_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_RESULT_IS_OK, .dest = t, .arg1 = result_temp};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_concat(IrFunc *f, int a_temp, int b_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CONCAT, .dest = t, .arg1 = a_temp, .arg2 = b_temp};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_result_or_fallback(IrFunc *f, int result_temp, int fallback_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_RESULT_OR_FALLBACK, .dest = t, .arg1 = result_temp, .arg2 = fallback_temp};
  emit(&f->instrs, ins);
  return t;
}

int ir_emit_call(IrFunc *f, const char *fname, int arg0_temp, int arg1_temp) {
  int t = ir_func_new_temp(f);
  IrInstr ins = {.op = IR_CALL, .dest = t, .arg1 = arg0_temp, .arg2 = arg1_temp, .s = fname ? strdup(fname) : NULL};
  emit(&f->instrs, ins);
  return t;
}

// ===== Validator =====
static int label_exists(const char *label, char **labels, size_t n) {
  for (size_t i = 0; i < n; ++i) if (strcmp(labels[i], label) == 0) return 1;
  return 0;
}

int ir_validate(const IrProgram *prog, char **errmsg) {
  if (!prog) return 0;
  for (size_t fi = 0; fi < prog->funcs.len; ++fi) {
    const IrFunc *f = &prog->funcs.items[fi];
    char **labels = NULL; size_t nlabels = 0, cap = 0;
    for (size_t i = 0; i < f->instrs.len; ++i) {
      const IrInstr *ins = &f->instrs.items[i];
      if (ins->op == IR_LABEL) {
        if (label_exists(ins->s, labels, nlabels)) {
          if (errmsg) {
            size_t len = snprintf(NULL, 0, "duplicate label %s in func %s", ins->s, f->name);
            *errmsg = malloc(len + 1);
            snprintf(*errmsg, len + 1, "duplicate label %s in func %s", ins->s, f->name);
          }
          free(labels);
          return 0;
        }
        if (nlabels == cap) { cap = cap ? cap * 2 : 8; labels = realloc(labels, cap * sizeof(char *)); }
        labels[nlabels++] = ins->s;
      }
    }
    for (size_t i = 0; i < f->instrs.len; ++i) {
      const IrInstr *ins = &f->instrs.items[i];
      if (ins->op == IR_JUMP || ins->op == IR_JUMP_IF_FALSE) {
        if (!label_exists(ins->s, labels, nlabels)) {
          if (errmsg) {
            size_t len = snprintf(NULL, 0, "missing label %s in func %s", ins->s, f->name);
            *errmsg = malloc(len + 1);
            snprintf(*errmsg, len + 1, "missing label %s in func %s", ins->s, f->name);
          }
          free(labels);
          return 0;
        }
      }
    }
    free(labels);
  }
  return 1;
}

// ===== Translator =====
#include "liminal/lexer.h"

static char *strndup0(const char *s, size_t len) {
  char *p = xmalloc(len + 1);
  memcpy(p, s, len);
  p[len] = '\0';
  return p;
}

static char *string_to_cstr(String s) { return strndup0(s.data, s.len); }

static char *unquote_string_literal(String s) {
  if (s.len >= 2 && s.data[0] == '\'' && s.data[s.len - 1] == '\'') {
    return strndup0(s.data + 1, s.len - 2);
  }
  return string_to_cstr(s);
}

static IrOp binop_to_ir(int op) {
  switch (op) {
  case TK_PLUS: return IR_ADD;
  case TK_MINUS: return IR_SUB;
  case TK_STAR: return IR_MUL;
  case TK_SLASH: case TK_DIV: return IR_DIV;
  case TK_MOD: return IR_MOD;
  case TK_EQ: return IR_EQ;
  case TK_NEQ: return IR_NEQ;
  case TK_LT: return IR_LT;
  case TK_GT: return IR_GT;
  case TK_LE: return IR_LE;
  case TK_GE: return IR_GE;
  case TK_AND: return IR_AND;
  case TK_OR: return IR_OR;
  default: return IR_NOP;
  }
}

static int lower_expr(IrFunc *f, const ASTExpr *e);
static void lower_stmt(IrFunc *f, const ASTStmt *s);

int ir_emit_make_result_ok(IrFunc *f, int arg_temp) {
  IrInstr ins = {.op = IR_MAKE_RESULT_OK, .dest = ir_func_new_temp(f), .arg1 = arg_temp};
  emit(&f->instrs, ins);
  return ins.dest;
}
int ir_emit_make_result_err(IrFunc *f, int arg_temp) {
  IrInstr ins = {.op = IR_MAKE_RESULT_ERR, .dest = ir_func_new_temp(f), .arg1 = arg_temp};
  emit(&f->instrs, ins);
  return ins.dest;
}

static int lower_expr(IrFunc *f, const ASTExpr *e) {
  if (!e) return -1;
  switch (e->kind) {
  case EXPR_LITERAL: {
    ASTLiteral lit = e->as.literal;
    switch (lit.literal_kind) {
    case TK_BOOL:
      return ir_emit_const_bool(f, strncasecmp(lit.value.data, "true", 4)==0 ? 1 : 0);
    case TK_INTEGER:
      return ir_emit_const_int(f, (int)strtol(lit.value.data, NULL, 10));
    case TK_REAL:
      return ir_emit_const_real(f, strtod(lit.value.data, NULL));
    case TK_STRING:
    case TK_CHAR: {
      char *s = unquote_string_literal(lit.value);
      int t = ir_emit_const_string(f, s);
      free(s);
      return t;
    }
    default:
      return ir_emit_const_int(f, 0);
    }
  }
  case EXPR_IDENT: {
    char *name = string_to_cstr(e->as.ident.name);
    if (getenv("LIMINAL_DEBUG_IR_LOG")) fprintf(stderr, "[ir] ident %s\n", name);
    if (strcmp(name, "Nothing") == 0) { free(name); return ir_emit_const_optional_none(f); }
    int t = ir_emit_load_var(f, name);
    free(name);
    return t;
  }
  case EXPR_INDEX: {
    if (e->as.index.base->kind == EXPR_IDENT && e->as.index.indices.len==1) {
      char *base = string_to_cstr(e->as.index.base->as.ident.name);
      int idx = lower_expr(f, e->as.index.indices.items[0]);
      // encode base name in s, arg2 is idx
      IrInstr ins = {.op=IR_INDEX, .dest=ir_func_new_temp(f), .arg2=idx, .s=base};
      emit(&f->instrs, ins);
      return ins.dest;
    }
    return ir_emit_const_int(f,0);
  }
  case EXPR_FIELD: {
    if (e->as.field.base->kind == EXPR_IDENT) {
      char *base = string_to_cstr(e->as.field.base->as.ident.name);
      char *fld = string_to_cstr(e->as.field.field);
      char buf[256]; snprintf(buf,sizeof(buf),"%s.%s", base, fld);
      free(base); free(fld);
      return ir_emit_load_var(f, buf);
    }
    return ir_emit_const_int(f,0);
  }
  case EXPR_ASK: {
    char *oracle_name = NULL;
    if (e->as.ask.oracle && e->as.ask.oracle->kind == EXPR_IDENT) {
      oracle_name = string_to_cstr(e->as.ask.oracle->as.ident.name);
    }
    char *schema_name = NULL;
    if (e->as.ask.into_type && e->as.ask.into_type->kind == TYPE_IDENT) {
      schema_name = string_to_cstr(e->as.ask.into_type->as.ident.name);
    }
    int prompt_t = lower_expr(f, e->as.ask.input);
    int fb_t = -1;
    if (e->as.ask.fallback) fb_t = lower_expr(f, e->as.ask.fallback);
    int t = ir_emit_ask(f, prompt_t, fb_t, oracle_name, schema_name);
    if (e->as.ask.with_cost && f->instrs.len > 0) {
      f->instrs.items[f->instrs.len-1].f = 1.0;
    }
    free(oracle_name);
    free(schema_name);
    return t;
  }
  case EXPR_CONSULT: {
    char *oracle_name = NULL;
    if (getenv("LIMINAL_DEBUG_IR_LOG")) fprintf(stderr, "[ir] consult expr\n");
    if (e->as.consult.oracle && e->as.consult.oracle->kind == EXPR_IDENT) {
      oracle_name = string_to_cstr(e->as.consult.oracle->as.ident.name);
    }
    char *schema_name = NULL;
    if (e->as.consult.into_type && e->as.consult.into_type->kind == TYPE_IDENT) {
      schema_name = string_to_cstr(e->as.consult.into_type->as.ident.name);
    }
    int prompt_t = lower_expr(f, e->as.consult.input);
    int fallback_t = -1;
    if (e->as.consult.fallback) fallback_t = lower_expr(f, e->as.consult.fallback);
    int hint_t = -1;
    if (e->as.consult.hint) hint_t = lower_expr(f, e->as.consult.hint);
    // vars: _consult_prompt_X, _consult_retries_X
    char bufp[64]; snprintf(bufp, sizeof(bufp), "__consult_prompt_%d", f->next_label);
    char bufr[64]; snprintf(bufr, sizeof(bufr), "__consult_retries_%d", f->next_label);
    ir_emit_store_var(f, bufp, prompt_t);
    int retries_t = ir_emit_const_int(f, e->as.consult.attempts > 0 ? e->as.consult.attempts : 1);
    ir_emit_store_var(f, bufr, retries_t);
    char loop_label[64]; snprintf(loop_label, sizeof(loop_label), "LCONSULT_%d", f->next_label++);
    char done_label[64]; snprintf(done_label, sizeof(done_label), "LCONSULT_DONE_%d", f->next_label++);
    ir_emit_label(f, loop_label);
    int curr_prompt_t = ir_emit_load_var(f, bufp);
    int res_t = ir_emit_ask(f, curr_prompt_t, -1, oracle_name, schema_name);
    int ok_t = ir_emit_result_is_ok(f, res_t);
    char retry_label[64]; snprintf(retry_label, sizeof(retry_label), "LCONSULT_RETRY_%d", f->next_label++);
    ir_emit_jump_if_false(f, ok_t, retry_label);
    ir_emit_jump(f, done_label);
    ir_emit_label(f, retry_label);
    int retries_val_t = ir_emit_load_var(f, bufr);
    int one_t = ir_emit_const_int(f, 1);
    int retries_dec_t = ir_emit_binop(f, IR_SUB, retries_val_t, one_t);
    ir_emit_store_var(f, bufr, retries_dec_t);
    int zero_t = ir_emit_const_int(f, 0);
    int has_more_t = ir_emit_binop(f, IR_GT, retries_dec_t, zero_t);
    ir_emit_jump_if_false(f, has_more_t, done_label);
    if (hint_t >= 0) {
      int nl_t = ir_emit_const_string(f, "\n\nHint: ");
      int tmp1_t = ir_emit_concat(f, curr_prompt_t, nl_t);
      int new_prompt_t = ir_emit_concat(f, tmp1_t, hint_t);
      ir_emit_store_var(f, bufp, new_prompt_t);
    }
    ir_emit_jump(f, loop_label);
    ir_emit_label(f, done_label);
    int out_t = res_t;
    if (fallback_t >= 0) out_t = ir_emit_result_or_fallback(f, res_t, fallback_t);
    free(oracle_name);
    free(schema_name);
    return out_t;
  }
  case EXPR_BINARY: {
    int lhs = lower_expr(f, e->as.binary.lhs);
    int rhs = lower_expr(f, e->as.binary.rhs);
    IrOp op = binop_to_ir(e->as.binary.op);
    return ir_emit_binop(f, op, lhs, rhs);
  }
  case EXPR_CONCAT: {
    int lhs = lower_expr(f, e->as.concat.lhs);
    int rhs = lower_expr(f, e->as.concat.rhs);
    return ir_emit_concat(f, lhs, rhs);
  }
  case EXPR_UNARY: {
    int inner = lower_expr(f, e->as.unary.expr);
    if (e->as.unary.op == TK_MINUS) {
      int zero = ir_emit_const_int(f, 0);
      return ir_emit_binop(f, IR_SUB, zero, inner);
    } else if (e->as.unary.op == TK_NOT) {
      int zero = ir_emit_const_int(f, 0);
      return ir_emit_binop(f, IR_EQ, inner, zero);
    }
    return inner;
  }
  case EXPR_CALL: {
    ASTExpr *callee = e->as.call.callee;
    // Builtins first (ident callee)
    if (callee->kind == EXPR_IDENT) {
      char *name = string_to_cstr(callee->as.ident.name);
      const char *dbg_call = getenv("LIMINAL_DEBUG_IR_CALL");
      if (dbg_call) fprintf(stderr, "[ir] call %s nargs=%zu\n", name, e->as.call.args.len);
      if (strcasecmp(name, "Ok") == 0 && e->as.call.args.len == 1) {
        int at = lower_expr(f, e->as.call.args.items[0]); free(name); return ir_emit_make_result_ok(f, at);
      }
      if (strcasecmp(name, "Err") == 0 && e->as.call.args.len == 1) {
        int at = lower_expr(f, e->as.call.args.items[0]); free(name); return ir_emit_make_result_err(f, at);
      }
      if (strcasecmp(name, "Write") == 0 || strcasecmp(name, "WriteLn") == 0) {
        int newline = (strcasecmp(name, "WriteLn") == 0);
        for (size_t i = 0; i < e->as.call.args.len; ++i) {
          int t = lower_expr(f, e->as.call.args.items[i]);
          ir_emit_print(f, t, 0);
        }
        if (newline) ir_emit_print(f, -1, 1);
        free(name);
        return ir_emit_const_int(f, 0);
      } else if (strcasecmp(name, "ReadLn") == 0) {
        if (e->as.call.args.len == 1 && e->as.call.args.items[0]->kind == EXPR_IDENT) {
          char *var = string_to_cstr(e->as.call.args.items[0]->as.ident.name);
          ir_emit_readln(f, var);
          free(var);
        }
        free(name);
        return ir_emit_const_int(f, 0);
      } else if (strcasecmp(name, "ReadFile") == 0) {
        if (e->as.call.args.len == 1) {
          int path = lower_expr(f, e->as.call.args.items[0]);
          free(name);
          return ir_emit_read_file(f, path);
        }
      } else if (strcasecmp(name, "WriteFile") == 0) {
        if (e->as.call.args.len == 2) {
          int path = lower_expr(f, e->as.call.args.items[0]);
          int content = lower_expr(f, e->as.call.args.items[1]);
          ir_emit_write_file(f, path, content);
          free(name);
          return ir_emit_const_int(f, 0);
        }
      } else if (strcasecmp(name, "Ask") == 0) {
        int prompt = -1;
        int fallback = -1;
        char *oracle = NULL;
        char *schema_name = NULL;
        if (e->as.call.args.len >= 1) prompt = lower_expr(f, e->as.call.args.items[0]);
        if (e->as.call.args.len >= 2) fallback = lower_expr(f, e->as.call.args.items[1]);
        if (e->as.call.args.len >= 3 && e->as.call.args.items[2]->kind == EXPR_IDENT) oracle = string_to_cstr(e->as.call.args.items[2]->as.ident.name);
        if (e->as.call.args.len >= 4 && e->as.call.args.items[3]->kind == EXPR_IDENT) schema_name = string_to_cstr(e->as.call.args.items[3]->as.ident.name);
        int t = ir_emit_ask(f, prompt, fallback, oracle, schema_name);
        free(oracle);
        free(schema_name);
        free(name);
        return t;
      }
      // Not a builtin: emit call
      int arg0 = -1;
      if (e->as.call.args.len > 0) arg0 = lower_expr(f, e->as.call.args.items[0]);
      int arg1 = -1;
      if (e->as.call.args.len > 1) arg1 = lower_expr(f, e->as.call.args.items[1]);
      int t = ir_emit_call(f, name, arg0, arg1);
      free(name);
      return t;
    }
    // handle method calls like result.UnwrapOr(x)
    if (callee->kind == EXPR_FIELD) {
      ASTExpr *base = callee->as.field.base;
      char *fname = string_to_cstr(callee->as.field.field);
      if (strcmp(fname, "UnwrapOr") == 0 && e->as.call.args.len == 1) {
        int res_t = lower_expr(f, base);
        int fb_t = lower_expr(f, e->as.call.args.items[0]);
        free(fname);
        return ir_emit_result_unwrap(f, res_t, fb_t);
      }
      free(fname);
    }
    return ir_emit_const_int(f, 0);
  }
  default:
    return ir_emit_const_int(f, 0);
  }
}

static char *fresh_label(IrFunc *f) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", f->next_label++);
  return strdup(buf);
}

int ir_emit_index(IrFunc *f, const char *base, int idx_temp) {
  IrInstr ins = {.op = IR_INDEX, .dest = ir_func_new_temp(f), .arg2 = idx_temp, .s = strdup(base)};
  emit(&f->instrs, ins);
  return ins.dest;
}

static void lower_stmt(IrFunc *f, const ASTStmt *s) {
  if (!s) return;
  switch (s->kind) {
  case STMT_ASSIGN: {
    ASTExpr *target = s->as.assign.target;
    if (getenv("LIMINAL_DEBUG_IR_LOG")) {
      if (target && target->kind==EXPR_IDENT) fprintf(stderr, "[ir] assign target ident %s\n", target->as.ident.name.data);
      else fprintf(stderr, "[ir] assign target kind %d\n", target ? (int)target->kind : -1);
    }
    if (target->kind == EXPR_FIELD) {
      if (target->as.field.base->kind==EXPR_IDENT) {
        char *base=string_to_cstr(target->as.field.base->as.ident.name);
        char *fld=string_to_cstr(target->as.field.field);
        char buf[256]; snprintf(buf,sizeof(buf),"%s.%s", base, fld);
        int val = lower_expr(f, s->as.assign.value);
        ir_emit_store_var(f, buf, val);
        free(base); free(fld);
      }
      break;
    }
    if (target->kind != EXPR_IDENT) return; // unsupported
    char *name = string_to_cstr(target->as.ident.name);
    if (s->as.assign.value->kind == EXPR_ARRAY) {
      // flatten array literal
      size_t len = s->as.assign.value->as.array.elements.len;
      for (size_t i=0;i<len;++i){
        ASTExpr *el = s->as.assign.value->as.array.elements.items[i];
        if (el->kind == EXPR_RECORD) {
          for (size_t fi=0; fi<el->as.record.fields.len; ++fi) {
            ASTField *fld = &el->as.record.fields.items[fi];
            int tv = lower_expr(f, fld->value);
            char buf[256]; snprintf(buf,sizeof(buf),"%s.%zu.%s", name, i, fld->key.data);
            ir_emit_store_var(f, buf, tv);
          }
          // optional: store placeholder for base element
          char buf0[256]; snprintf(buf0,sizeof(buf0),"%s.%zu", name, i);
          int zero = ir_emit_const_int(f,0); ir_emit_store_var(f, buf0, zero);
        } else {
          int tv=lower_expr(f, el);
          char buf[256]; snprintf(buf,sizeof(buf),"%s.%zu", name, i);
          ir_emit_store_var(f, buf, tv);
        }
      }
      int zero = ir_emit_const_int(f,0); ir_emit_store_var(f, name, zero);
      char buflen[256]; snprintf(buflen,sizeof(buflen),"%s.len", name); int len_t = ir_emit_const_int(f, (int)len); ir_emit_store_var(f, buflen, len_t);
      free(name);
      break;
    }
    int val = lower_expr(f, s->as.assign.value);
    ir_emit_store_var(f, name, val);
    free(name);
    break;
  }
  case STMT_EXPR:
    lower_expr(f, s->as.expr_stmt.expr);
    break;
  case STMT_BLOCK:
    if (getenv("LIMINAL_DEBUG_IR_LOG")) fprintf(stderr, "[ir] block stmts=%zu\n", s->as.block.stmts.len);
    for (size_t i = 0; i < s->as.block.stmts.len; ++i) lower_stmt(f, s->as.block.stmts.items[i]);
    break;
  case STMT_IF: {
    int cond = lower_expr(f, s->as.if_stmt.cond);
    char *label_else = fresh_label(f);
    char *label_end = fresh_label(f);
    ir_emit_jump_if_false(f, cond, label_else);
    lower_stmt(f, s->as.if_stmt.then_branch);
    ir_emit_jump(f, label_end);
    ir_emit_label(f, label_else);
    if (s->as.if_stmt.else_branch) lower_stmt(f, s->as.if_stmt.else_branch);
    ir_emit_label(f, label_end);
    free(label_else); free(label_end);
    break;
  }
  case STMT_WHILE: {
    char *label_loop = fresh_label(f);
    char *label_end = fresh_label(f);
    ir_emit_label(f, label_loop);
    int cond = lower_expr(f, s->as.while_stmt.cond);
    ir_emit_jump_if_false(f, cond, label_end);
    lower_stmt(f, s->as.while_stmt.body);
    ir_emit_jump(f, label_loop);
    ir_emit_label(f, label_end);
    free(label_loop); free(label_end);
    break;
  }
  case STMT_REPEAT: {
    char *label_loop = fresh_label(f);
    ir_emit_label(f, label_loop);
    lower_stmt(f, s->as.repeat_stmt.body);
    int cond = lower_expr(f, s->as.repeat_stmt.cond);
    ir_emit_jump_if_false(f, cond, label_loop);
    free(label_loop);
    break;
  }
  case STMT_CASE: {
    int expr_t = lower_expr(f, s->as.case_stmt.expr);
    char *label_end = fresh_label(f);
    for (size_t i=0;i<s->as.case_stmt.patterns.len;++i){
      char *lbl = fresh_label(f);
      ASTExpr *pat = s->as.case_stmt.patterns.items[i];
      if (pat->kind==EXPR_CALL && pat->as.call.callee && pat->as.call.callee->kind==EXPR_IDENT) {
        String nm = pat->as.call.callee->as.ident.name;
        if (nm.data && strncasecmp(nm.data,"Ok",nm.len)==0 && pat->as.call.args.len==1 && pat->as.call.args.items[0]->kind==EXPR_IDENT) {
          int ok_t = ir_emit_result_is_ok(f, expr_t);
          ir_emit_jump_if_false(f, ok_t, lbl);
          int inner_t = ir_emit_result_unwrap(f, expr_t, -1);
          char *varname = string_to_cstr(pat->as.call.args.items[0]->as.ident.name);
          ir_emit_store_var(f, varname, inner_t);
          free(varname);
          lower_stmt(f, s->as.case_stmt.branches.items[i]);
          ir_emit_jump(f, label_end);
          ir_emit_label(f, lbl);
          free(lbl);
          continue;
        }
        if (nm.data && strncasecmp(nm.data,"Err",nm.len)==0 && pat->as.call.args.len==1 && pat->as.call.args.items[0]->kind==EXPR_IDENT) {
          int ok_t = ir_emit_result_is_ok(f, expr_t);
          int zero = ir_emit_const_int(f,0);
          int cmp = ir_emit_binop(f, IR_EQ, ok_t, zero);
          ir_emit_jump_if_false(f, cmp, lbl);
          int err_t = ir_emit_result_unwrap_err(f, expr_t);
          char *varname = string_to_cstr(pat->as.call.args.items[0]->as.ident.name);
          ir_emit_store_var(f, varname, err_t);
          free(varname);
          lower_stmt(f, s->as.case_stmt.branches.items[i]);
          ir_emit_jump(f, label_end);
          ir_emit_label(f, lbl);
          free(lbl);
          continue;
        }
      }
      int pat_t = lower_expr(f, pat);
      int cmp_t = ir_emit_binop(f, IR_EQ, expr_t, pat_t);
      ir_emit_jump_if_false(f, cmp_t, lbl);
      lower_stmt(f, s->as.case_stmt.branches.items[i]);
      ir_emit_jump(f, label_end);
      ir_emit_label(f, lbl);
      free(lbl);
    }
    if (s->as.case_stmt.else_branch) lower_stmt(f, s->as.case_stmt.else_branch);
    ir_emit_label(f, label_end);
    free(label_end);
    break;
  }
  case STMT_FOR_IN: {
    if (s->as.for_in_stmt.iterable->kind == EXPR_IDENT) {
      char *iter = string_to_cstr(s->as.for_in_stmt.iterable->as.ident.name);
      char buflen[256]; snprintf(buflen,sizeof(buflen),"%s.len", iter);
      int len_t = ir_emit_load_var(f, buflen);
      char idxname[256]; snprintf(idxname,sizeof(idxname),"__idx_%d", f->next_label);
      int zero = ir_emit_const_int(f,0); ir_emit_store_var(f, idxname, zero);
      char *label_loop = fresh_label(f);
      char *label_end = fresh_label(f);
      ir_emit_label(f, label_loop);
      int cur_idx = ir_emit_load_var(f, idxname);
      int cmp_t = ir_emit_binop(f, IR_LT, cur_idx, len_t);
      ir_emit_jump_if_false(f, cmp_t, label_end);
      int elem_t = ir_emit_index(f, iter, cur_idx);
      char *varname = string_to_cstr(s->as.for_in_stmt.var.name);
      ir_emit_store_var(f, varname, elem_t);
      free(varname);
      lower_stmt(f, s->as.for_in_stmt.body);
      int one = ir_emit_const_int(f,1);
      int inc = ir_emit_binop(f, IR_ADD, cur_idx, one);
      ir_emit_store_var(f, idxname, inc);
      ir_emit_jump(f, label_loop);
      ir_emit_label(f, label_end);
      free(iter); free(label_loop); free(label_end);
    }
    break;
  }
  case STMT_FOR: {
    char *label_loop = fresh_label(f);
    char *label_end = fresh_label(f);
    char *varname = string_to_cstr(s->as.for_stmt.var.name);
    int init_t = lower_expr(f, s->as.for_stmt.init);
    int limit_t = lower_expr(f, s->as.for_stmt.to);
    ir_emit_store_var(f, varname, init_t);
    ir_emit_label(f, label_loop);
    int cur_t = ir_emit_load_var(f, varname);
    int cmp_t = ir_emit_binop(f, s->as.for_stmt.descending ? IR_GE : IR_LE, cur_t, limit_t);
    ir_emit_jump_if_false(f, cmp_t, label_end);
    lower_stmt(f, s->as.for_stmt.body);
    int one_t = ir_emit_const_int(f, 1);
    int next_t = ir_emit_binop(f, s->as.for_stmt.descending ? IR_SUB : IR_ADD, cur_t, one_t);
    ir_emit_store_var(f, varname, next_t);
    ir_emit_jump(f, label_loop);
    ir_emit_label(f, label_end);
    free(varname);
    free(label_loop); free(label_end);
    break;
  }
  case STMT_RETURN: {
    int val = lower_expr(f, s->as.return_stmt.value);
    ir_emit_ret(f, val);
    break;
  }
  default:
    /* unsupported */
    break;
  }
}

static Type *ir_type_from_ast(Symtab *st, ASTType *ty);

static Type *ir_type_builtin(const char *name) {
  if (strcasecmp(name, "Integer") == 0) return type_primitive(TYPEK_INT);
  if (strcasecmp(name, "Real") == 0) return type_primitive(TYPEK_REAL);
  if (strcasecmp(name, "Boolean") == 0) return type_primitive(TYPEK_BOOL);
  if (strcasecmp(name, "String") == 0) return type_primitive(TYPEK_STRING);
  if (strcasecmp(name, "Bytes") == 0) return type_primitive(TYPEK_BYTES);
  if (strcasecmp(name, "Char") == 0) return type_primitive(TYPEK_CHAR);
  if (strcasecmp(name, "Byte") == 0) return type_primitive(TYPEK_BYTE);
  return NULL;
}

static Type *ir_type_from_ast(Symtab *st, ASTType *ty) {
  if (!ty) return type_primitive(TYPEK_UNKNOWN);
  switch (ty->kind) {
  case TYPE_IDENT: {
    Symbol *sym = symtab_lookup(st, ty->as.ident.name.data);
    if (sym && sym->kind == SYM_TYPE) return sym->type;
    Type *b = ir_type_builtin(ty->as.ident.name.data);
    return b ? b : type_primitive(TYPEK_UNKNOWN);
  }
  case TYPE_ARRAY:
    return type_array(ir_type_from_ast(st, ty->as.array_type.elem));
  case TYPE_TUPLE: {
    TypeVec vec={0};
    for (size_t i=0;i<ty->as.tuple_type.elements.len;++i) typevec_push(&vec, ir_type_from_ast(st, ty->as.tuple_type.elements.items[i]));
    return type_tuple(vec);
  }
  case TYPE_SCHEMA: {
    Type *sch = type_schema(NULL);
    // fields handled elsewhere
    return sch;
  }
  default:
    return type_primitive(TYPEK_UNKNOWN);
  }
}

static void ir_collect_schemas(IrProgram *p, const ASTNode *node) {
  Symtab *st = symtab_create();
  // pass 1: declare schema names
  for (size_t i = 0; i < node->as.program.types.len; ++i) {
    ASTNode *td = node->as.program.types.items[i];
    if (td->kind == AST_TYPE_DECL && td->as.type_decl.type->kind == TYPE_SCHEMA) {
      Type *schema = type_schema(td->as.type_decl.name.data);
      symtab_define(st, SYM_TYPE, td->as.type_decl.name.data, schema);
      typevec_push(&p->schemas, schema);
    }
  }
  // pass 2: populate fields
  for (size_t i = 0; i < node->as.program.types.len; ++i) {
    ASTNode *td = node->as.program.types.items[i];
    if (td->kind == AST_TYPE_DECL && td->as.type_decl.type->kind == TYPE_SCHEMA) {
      Type *schema = symtab_lookup(st, td->as.type_decl.name.data)->type;
      ASTType *stype = td->as.type_decl.type;
      for (size_t j = 0; j < stype->as.schema_type.fields.len; ++j) {
        ASTFieldDecl *fd = &stype->as.schema_type.fields.items[j];
        Type *ft = ir_type_from_ast(st, fd->type);
        schema_add_field(schema, fd->name.data, ft);
      }
    }
  }
  symtab_destroy(st);
}

IrProgram *ir_from_ast(const ASTNode *node) {
  if (!node || node->kind != AST_PROGRAM) return NULL;
  IrProgram *p = ir_program_new();
  ir_collect_schemas(p, node);
  char *prog_name = string_to_cstr(node->as.program.name);
  IrFunc mainf = ir_func_create(prog_name);
  free(prog_name);
  mainf.next_label = 0;
  // initialize enums as constants
  for (size_t i=0;i<node->as.program.types.len;++i){
    ASTNode *td=node->as.program.types.items[i];
    if (td->as.type_decl.type && td->as.type_decl.type->kind==TYPE_ENUM){
      ASTType *et=td->as.type_decl.type;
      for (size_t ei=0; ei<et->as.enum_type.len; ++ei){
        String nm = et->as.enum_type.items[ei];
        char *nm_c = string_to_cstr(nm);
        int c = ir_emit_const_int(&mainf, (int)ei);
        ir_emit_store_var(&mainf, nm_c, c);
        free(nm_c);
      }
    }
  }
  lower_stmt(&mainf, node->as.program.body);
  ir_program_add_func(p, mainf);
  for (size_t i = 0; i < node->as.program.functions.len; ++i) {
    ASTNode *fn_node = node->as.program.functions.items[i];
    if (fn_node->kind != AST_FUNC_DECL) continue;
    char *fname = string_to_cstr(fn_node->as.func_decl.name);
    IrFunc f = ir_func_create(fname);
    free(fname);
    ASTFunction *afn = &fn_node->as.func_decl;
    if (afn->params.len > 0) {
      f.param_count = (int)afn->params.len;
      f.params = calloc(f.param_count, sizeof(char*));
      for (int pi = 0; pi < f.param_count; ++pi) {
        f.params[pi] = string_to_cstr(afn->params.items[pi].name);
      }
    }
    f.next_label = 0;
    lower_stmt(&f, fn_node->as.func_decl.body);
    ir_program_add_func(p, f);
  }
  return p;
}
