#define _POSIX_C_SOURCE 200809L
#include "liminal/typecheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static ASTNode *tc_prog = NULL;

static char *string_to_cstr_local(String s){ if (!s.data) return strdup(""); return strndup(s.data, s.len); }

static void add_error(TypeCheckResult *res, LiminalSpan span, const char *msg) {
  if (res->errors.len == res->errors.cap) {
    res->errors.cap = res->errors.cap ? res->errors.cap * 2 : 4;
    res->errors.items = realloc(res->errors.items, res->errors.cap * sizeof(TypeCheckError));
  }
  res->errors.items[res->errors.len].span = span;
  res->errors.items[res->errors.len].message = strdup(msg);
  res->errors.len++;
  res->ok = 0;
}

static Type *builtin_primitive(const char *name) {
  if (strcasecmp(name, "Integer") == 0) return type_primitive(TYPEK_INT);
  if (strcasecmp(name, "Real") == 0) return type_primitive(TYPEK_REAL);
  if (strcasecmp(name, "Boolean") == 0) return type_primitive(TYPEK_BOOL);
  if (strcasecmp(name, "String") == 0) return type_primitive(TYPEK_STRING);
  if (strcasecmp(name, "Bytes") == 0) return type_primitive(TYPEK_BYTES);
  if (strcasecmp(name, "Char") == 0) return type_primitive(TYPEK_CHAR);
  if (strcasecmp(name, "Byte") == 0) return type_primitive(TYPEK_BYTE);
  return NULL;
}

static Type *resolve_type_ident(Symtab *st, const char *name) {
  if (!name) return type_primitive(TYPEK_UNKNOWN);
  Symbol *sym = symtab_lookup(st, name);
  if (sym && sym->kind == SYM_TYPE) return sym->type;
  return builtin_primitive(name);
}

static Type *type_from_ast(Symtab *st, ASTType *ty) {
  if (!ty) return type_primitive(TYPEK_UNKNOWN);
  switch (ty->kind) {
  case TYPE_RECORD: {
    Type *rec = type_schema(NULL);
    rec->kind = TYPEK_RECORD;
    for (size_t i = 0; i < ty->as.record_type.fields.len; ++i) {
      ASTFieldDecl *fd = &ty->as.record_type.fields.items[i];
      Type *ft = type_from_ast(st, fd->type);
      schema_add_field(rec, fd->name.data, ft);
    }
    return rec;
  }
  case TYPE_OPTIONAL: {
    return type_optional(type_from_ast(st, ty->as.optional_type.option));
  }
  case TYPE_CONSTRAINED: {
    // treat constraints as underlying base type
    if (ty->as.constrained_type.base.data) {
      char *cname = string_to_cstr_local(ty->as.constrained_type.base);
      Type *base = resolve_type_ident(st, cname);
      free(cname);
      return base ? base : type_primitive(TYPEK_UNKNOWN);
    }
    return type_primitive(TYPEK_UNKNOWN);
  }
  case TYPE_ENUM: {
    Type *enm = type_schema(NULL);
    enm->kind = TYPEK_ENUM;
    for (size_t i=0;i<ty->as.enum_type.len;++i){
      String *nm = &ty->as.enum_type.items[i];
      schema_add_field(enm, nm->data, type_primitive(TYPEK_INT));
    }
    return enm;
  }
  case TYPE_RESULT: {
    Type *ok = type_from_ast(st, ty->as.result_type.ok);
    Type *err = ty->as.result_type.err ? type_from_ast(st, ty->as.result_type.err) : type_primitive(TYPEK_STRING);
    return type_result(ok, err);
  }
  case TYPE_IDENT:
    {
      char *cname = string_to_cstr_local(ty->as.ident.name);
      Type *resolved = resolve_type_ident(st, cname);
      free(cname);
      return resolved ? resolved : type_primitive(TYPEK_UNKNOWN);
    }
  case TYPE_ARRAY:
    return type_array(type_from_ast(st, ty->as.array_type.elem));
  case TYPE_TUPLE: {
    TypeVec vec = {0};
    for (size_t i = 0; i < ty->as.tuple_type.elements.len; ++i) {
      typevec_push(&vec, type_from_ast(st, ty->as.tuple_type.elements.items[i]));
    }
    return type_tuple(vec);
  }
  case TYPE_SCHEMA: {
    Type *schema = type_schema(ty->as.schema_type.name.data);
    for (size_t i = 0; i < ty->as.schema_type.fields.len; ++i) {
      ASTFieldDecl *fd = &ty->as.schema_type.fields.items[i];
      Type *ft = type_from_ast(st, fd->type);
      schema_add_field(schema, fd->name.data, ft);
    }
    return schema;
  }
  default:
    return type_primitive(TYPEK_UNKNOWN);
  }
}

static Type *type_of_literal(TokenKind kind) {
  switch (kind) {
  case TK_INTEGER: return type_primitive(TYPEK_INT);
  case TK_REAL: return type_primitive(TYPEK_REAL);
  case TK_STRING: return type_primitive(TYPEK_STRING);
  case TK_BYTES: return type_primitive(TYPEK_BYTES);
  case TK_CHAR: return type_primitive(TYPEK_CHAR);
  case TK_FSTRING: return type_primitive(TYPEK_STRING);
  case TK_BOOL: return type_primitive(TYPEK_BOOL);
  default: return type_primitive(TYPEK_UNKNOWN);
  }
}


static Type *typecheck_expr(Symtab *st, TypeCheckResult *res, ASTExpr *e) {
  if (!e) return type_primitive(TYPEK_UNKNOWN);
  switch (e->kind) {
  case EXPR_LITERAL:
    return type_of_literal(e->as.literal.literal_kind);
  case EXPR_IDENT: {
    if (!e->as.ident.name.data) {
      add_error(res, e->span, "Undeclared identifier <null>");
      return type_primitive(TYPEK_UNKNOWN);
    }
    char *cname = string_to_cstr_local(e->as.ident.name);
    Symbol *sym = symtab_lookup(st, cname);
    free(cname);
    if (!sym) {
      if (strncasecmp(e->as.ident.name.data, "Nothing", e->as.ident.name.len) == 0) {
        return type_optional(type_primitive(TYPEK_UNKNOWN));
      }
      char buf[128]; snprintf(buf, sizeof(buf), "Undeclared identifier %s", e->as.ident.name.data);
      add_error(res, e->span, buf);
      return type_primitive(TYPEK_UNKNOWN);
    }
    return sym->type;
  }
  case EXPR_UNARY: {
    Type *t = typecheck_expr(st, res, e->as.unary.expr);
    return t;
  }
  case EXPR_BINARY: {
    Type *lt = typecheck_expr(st, res, e->as.binary.lhs);
    Type *rt = typecheck_expr(st, res, e->as.binary.rhs);
    if (getenv("LIMINAL_DEBUG_TC")) {
      char *ls = type_to_string(lt); char *rs = type_to_string(rt);
      fprintf(stderr, "[tc] binop %d : %s , %s\n", e->as.binary.op, ls, rs);
      free(ls); free(rs);
    }
    switch (e->as.binary.op) {
    case TK_PLUS:
      if (lt->kind == TYPEK_STRING && (rt->kind == TYPEK_STRING || rt->kind == TYPEK_CHAR)) return type_primitive(TYPEK_STRING);
      if (rt->kind == TYPEK_STRING && (lt->kind == TYPEK_STRING || lt->kind == TYPEK_CHAR)) return type_primitive(TYPEK_STRING);
      if ((lt->kind != TYPEK_INT && lt->kind != TYPEK_REAL) || (rt->kind != TYPEK_INT && rt->kind != TYPEK_REAL)) {
        add_error(res, e->span, "Arithmetic on non-numeric");
        return type_primitive(TYPEK_UNKNOWN);
      }
      if (lt->kind == TYPEK_REAL || rt->kind == TYPEK_REAL) return type_primitive(TYPEK_REAL);
      return type_primitive(TYPEK_INT);
    case TK_MINUS: case TK_STAR: case TK_SLASH: case TK_DIV: case TK_MOD:
      if ((lt->kind != TYPEK_INT && lt->kind != TYPEK_REAL) || (rt->kind != TYPEK_INT && rt->kind != TYPEK_REAL)) {
        add_error(res, e->span, "Arithmetic on non-numeric");
        return type_primitive(TYPEK_UNKNOWN);
      }
      if (lt->kind == TYPEK_REAL || rt->kind == TYPEK_REAL) return type_primitive(TYPEK_REAL);
      return type_primitive(TYPEK_INT);
    case TK_EQ: case TK_NEQ: case TK_LT: case TK_GT: case TK_LE: case TK_GE:
      return type_primitive(TYPEK_BOOL);
    case TK_AND: case TK_OR:
      return type_primitive(TYPEK_BOOL);
    default:
      return type_primitive(TYPEK_UNKNOWN);
    }
  }
  case EXPR_CALL:
    if (e->as.call.callee) {
      if (e->as.call.callee->kind == EXPR_FIELD) {
        String fld = e->as.call.callee->as.field.field;
        if (fld.data && strncasecmp(fld.data, "UnwrapOr", fld.len) == 0 && e->as.call.args.len == 1) {
          return type_primitive(TYPEK_STRING);
        }
      } else if (e->as.call.callee->kind == EXPR_IDENT) {
        String name = e->as.call.callee->as.ident.name;
        if (name.data && strncasecmp(name.data, "ReadFile", name.len) == 0) {
          return type_primitive(TYPEK_STRING);
        }
        if (name.data && strncasecmp(name.data, "Ok", name.len)==0 && e->as.call.args.len==1) {
          Type *argt = typecheck_expr(st, res, e->as.call.args.items[0]);
          Type *tr = type_result(argt, type_primitive(TYPEK_STRING));
          typevec_push(&res->temp_types, tr);
          return tr;
        }
        if (name.data && strncasecmp(name.data, "Err", name.len)==0 && e->as.call.args.len==1) {
          Type *tr = type_result(type_primitive(TYPEK_UNKNOWN), type_primitive(TYPEK_STRING));
          typevec_push(&res->temp_types, tr);
          return tr;
        }
        char *cname = string_to_cstr_local(name);
        Symbol *fsym = symtab_lookup(st, cname);
        free(cname);
        if (fsym) return fsym->type;
        if (tc_prog) {
          if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] fallback functions len=%zu\n", tc_prog->as.program.functions.len);
          for (size_t fi=0; fi<tc_prog->as.program.functions.len; ++fi) {
            ASTNode *fn = tc_prog->as.program.functions.items[fi];
            if (fn->as.func_decl.name.data && name.data && strcasecmp(fn->as.func_decl.name.data, name.data)==0) {
              if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] fallback fn match %.*s\n", (int)name.len, name.data);
              if (fn->as.func_decl.result_type) {
                Type *ft = type_from_ast(st, fn->as.func_decl.result_type);
                typevec_push(&res->temp_types, ft);
                return ft;
              }
            }
          }
        }
        if (getenv("LIMINAL_DEBUG_TC")) {
          fprintf(stderr,"[tc] call lookup failed: %.*s\n", (int)name.len, name.data);
        }
      }
    }
    return type_primitive(TYPEK_UNKNOWN);
  case EXPR_INDEX:
    // array indexing returns element type
    if (e->as.index.base) {
      Type *bt = typecheck_expr(st, res, e->as.index.base);
      if (bt->kind == TYPEK_ARRAY) return bt->as.array.elem;
    }
    return type_primitive(TYPEK_UNKNOWN);
  case EXPR_TUPLE: {
    TypeVec vec = {0};
    for (size_t i = 0; i < e->as.tuple.elements.len; ++i) {
      typevec_push(&vec, typecheck_expr(st, res, e->as.tuple.elements.items[i]));
    }
    Type *tt = type_tuple(vec);
    typevec_push(&res->temp_types, tt);
    return tt;
  }
  case EXPR_ARRAY: {
    // array literals: ensure homogeneous
    Type *elem = NULL;
    for (size_t i = 0; i < e->as.array.elements.len; ++i) {
      ASTExpr *el = e->as.array.elements.items[i];
      if (getenv("LIMINAL_DEBUG_TC")) { fprintf(stderr, "[tc] array elem kind=%d\n", el->kind); }
      Type *t = typecheck_expr(st, res, el);
      if (getenv("LIMINAL_DEBUG_TC")) { char *ts = type_to_string(t); fprintf(stderr, "[tc] array elem %zu: %s\n", i, ts); free(ts); }
      if (!elem) elem = t;
      else if (!type_equals(elem, t)) add_error(res, e->span, "Array elements must be same type");
    }
    if (!elem) elem = type_primitive(TYPEK_UNKNOWN);
    Type *arr = type_array(elem);
    if (getenv("LIMINAL_DEBUG_TC")) { char *es = type_to_string(elem); char *as = type_to_string(arr); fprintf(stderr, "[tc] array elem type: %s arr: %s\n", es, as); free(es); free(as);} 
    typevec_push(&res->temp_types, arr);
    return arr;
  }
  case EXPR_RECORD: {
    // record literals: build anonymous record type
    Type *rec = type_schema(NULL);
    for (size_t i=0;i<e->as.record.fields.len;++i){ ASTField *f=&e->as.record.fields.items[i]; Type *ft=typecheck_expr(st,res,f->value); schema_add_field(rec, f->key.data, ft);} 
    rec->kind = TYPEK_RECORD;
    typevec_push(&res->temp_types, rec);
    return rec;
  }
  case EXPR_FIELD: {
    Type *bt = typecheck_expr(st, res, e->as.field.base);
    if (getenv("LIMINAL_DEBUG_TC")) { char *bs = type_to_string(bt); fprintf(stderr, "[tc] field base type=%s\n", bs); free(bs);} 
    if (bt && (bt->kind==TYPEK_SCHEMA || bt->kind==TYPEK_RECORD)){
      char key[128]; snprintf(key,sizeof(key),"%.*s", (int)e->as.field.field.len, e->as.field.field.data);
      if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] field lookup %s\n", key);
      SchemaField *sf = schema_find_field(bt, key);
      if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] field found? %p\n", (void*)sf);
      if (sf) return sf->type;
    }
    return type_primitive(TYPEK_UNKNOWN);
  }
  case EXPR_ASK:
    if (e->as.ask.into_type) return type_from_ast(st, e->as.ask.into_type);
    return type_primitive(TYPEK_STRING);
  case EXPR_CONSULT:
    if (e->as.consult.into_type) return type_from_ast(st, e->as.consult.into_type);
    return type_primitive(TYPEK_STRING);
  case EXPR_CONCAT:
    // string concatenation
    typecheck_expr(st, res, e->as.concat.lhs);
    typecheck_expr(st, res, e->as.concat.rhs);
    return type_primitive(TYPEK_STRING);
  case EXPR_EMBED:
    return type_primitive(TYPEK_BYTES);
  case EXPR_CONTEXT:
    return type_primitive(TYPEK_UNKNOWN);
  default:
    return type_primitive(TYPEK_UNKNOWN);
  }
}

static void typecheck_stmt(Symtab *st, TypeCheckResult *res, ASTStmt *s) {
  if (!s) return;
  switch (s->kind) {
  case STMT_ASSIGN: {
    Type *lt = typecheck_expr(st, res, s->as.assign.target);
    Type *rt = typecheck_expr(st, res, s->as.assign.value);
    if (!type_equals(lt, rt)) {
      int ok = 0;
      if (lt && lt->kind == TYPEK_STRING && rt && rt->kind == TYPEK_CHAR) {
        ok = 1;
      }
      if (lt && lt->kind == TYPEK_OPTIONAL) {
        if (type_equals(lt->as.optional.inner, rt)) ok = 1;
        else if (rt && rt->kind == TYPEK_OPTIONAL) {
          if (rt->as.optional.inner && rt->as.optional.inner->kind == TYPEK_UNKNOWN) ok = 1;
          else if (type_equals(lt, rt)) ok = 1;
        }
      }
      if (lt && lt->kind == TYPEK_RESULT) {
        if (rt && rt->kind == TYPEK_RESULT) {
          Type *lok = lt->as.result.ok; Type *rok = rt->as.result.ok;
          Type *ler = lt->as.result.err; Type *rer = rt->as.result.err;
          if ((type_equals(lok, rok) || (rok && rok->kind==TYPEK_UNKNOWN)) && (type_equals(ler, rer) || (rer && rer->kind==TYPEK_UNKNOWN))) ok = 1;
        }
        // allow !T := !Unknown (Err)
        if (rt && rt->kind == TYPEK_RESULT && rt->as.result.ok && rt->as.result.ok->kind == TYPEK_UNKNOWN) ok = 1;
      }
      if (!ok) {
        char *ls = type_to_string(lt);
        char *rs = type_to_string(rt);
        char buf[256]; snprintf(buf, sizeof(buf), "Type mismatch: %s := %s", ls, rs);
        add_error(res, s->span, buf);
        free(ls); free(rs);
      }
    }
    break;
  }
  case STMT_EXPR:
    typecheck_expr(st, res, s->as.expr_stmt.expr);
    break;
  case STMT_IF:
    typecheck_expr(st, res, s->as.if_stmt.cond);
    typecheck_stmt(st, res, s->as.if_stmt.then_branch);
    typecheck_stmt(st, res, s->as.if_stmt.else_branch);
    break;
  case STMT_WHILE:
    typecheck_expr(st, res, s->as.while_stmt.cond);
    typecheck_stmt(st, res, s->as.while_stmt.body);
    break;
  case STMT_REPEAT:
    typecheck_stmt(st, res, s->as.repeat_stmt.body);
    typecheck_expr(st, res, s->as.repeat_stmt.cond);
    break;
  case STMT_FOR: {
    // ensure loop var exists; if not, define int
    Symbol *sym = symtab_lookup(st, s->as.for_stmt.var.name.data);
    if (!sym) {
      symtab_define(st, SYM_VAR, s->as.for_stmt.var.name.data, type_primitive(TYPEK_INT));
    }
    typecheck_expr(st, res, s->as.for_stmt.init);
    typecheck_expr(st, res, s->as.for_stmt.to);
    typecheck_stmt(st, res, s->as.for_stmt.body);
    break; }
  case STMT_FOR_IN: {
    Symbol *sym = symtab_lookup(st, s->as.for_in_stmt.var.name.data);
    if (!sym) symtab_define(st, SYM_VAR, s->as.for_in_stmt.var.name.data, type_primitive(TYPEK_INT));
    typecheck_expr(st, res, s->as.for_in_stmt.iterable);
    typecheck_stmt(st, res, s->as.for_in_stmt.body);
    break; }
  case STMT_BLOCK:
    symtab_push(st);
    for (size_t i = 0; i < s->as.block.stmts.len; ++i) typecheck_stmt(st, res, s->as.block.stmts.items[i]);
    symtab_pop(st);
    break;
  default:
    break;
  }
}

static void define_types(Symtab *st, ASTNode *prog, TypeVec *owned_types) {
  // Builtins already implicitly handled in resolve_type_ident
  for (size_t i = 0; i < prog->as.program.types.len; ++i) {
    ASTNode *td = prog->as.program.types.items[i];
    Type *ty = type_from_ast(st, td->as.type_decl.type);
    char *cname = string_to_cstr_local(td->as.type_decl.name);
    symtab_define(st, SYM_TYPE, cname, ty);
    free(cname);
    if (ty && ty->kind==TYPEK_ENUM) {
      for (size_t fi=0; fi<ty->as.schema.len; ++fi) {
        SchemaField *sf=&ty->as.schema.items[fi]; symtab_define(st, SYM_VAR, sf->name, type_primitive(TYPEK_INT));
      }
    }
    if (owned_types) typevec_push(owned_types, ty);
  }
}

static void define_globals(Symtab *st, ASTNode *prog, TypeVec *owned_types) {
  for (size_t i = 0; i < prog->as.program.vars.len; ++i) {
    ASTNode *vd = prog->as.program.vars.items[i];
    Type *ty = type_from_ast(st, vd->as.var_decl.type);
    char *cname = string_to_cstr_local(vd->as.var_decl.name);
    symtab_define(st, SYM_VAR, cname, ty);
    free(cname);
    if (owned_types) {
      int dup = 0;
      for (size_t oi=0; oi<owned_types->len; ++oi) if (owned_types->items[oi]==ty) { dup=1; break; }
      if (!dup) typevec_push(owned_types, ty);
    }
    // flatten record fields
    if (ty && (ty->kind==TYPEK_SCHEMA || ty->kind==TYPEK_RECORD)){
      for (size_t fi=0; fi<ty->as.schema.len; ++fi){ SchemaField *sf=&ty->as.schema.items[fi]; char buf[256]; snprintf(buf,sizeof(buf),"%s.%s", vd->as.var_decl.name.data, sf->name); symtab_define(st,SYM_VAR, buf, sf->type); }
    }
  }
}

static void define_functions(Symtab *st, ASTNode *prog, TypeVec *owned_types) {
  for (size_t i = 0; i < prog->as.program.functions.len; ++i) {
    ASTNode *fn = prog->as.program.functions.items[i];
    Type *fty = type_primitive(TYPEK_UNKNOWN);
    if (fn->as.func_decl.result_type) fty = type_from_ast(st, fn->as.func_decl.result_type);
    char *cname = string_to_cstr_local(fn->as.func_decl.name);
    symtab_define(st, SYM_FUNC, cname, fty);
    free(cname);
    if (owned_types && fty) {
      int dup = 0;
      for (size_t oi=0; oi<owned_types->len; ++oi) if (owned_types->items[oi]==fty) { dup=1; break; }
      if (!dup) typevec_push(owned_types, fty);
    }
  }
}

TypeCheckResult typecheck_program(ASTNode *prog) {
  TypeCheckResult res = {.ok = 1, .errors = {0}, .temp_types = {0}, .owned_types = {0} };
  tc_prog = prog;
  Symtab *st = symtab_create();
  if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] functions len=%zu\n", prog->as.program.functions.len);
  if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] types len=%zu\n", prog->as.program.types.len);
  define_types(st, prog, &res.owned_types);
  define_globals(st, prog, &res.owned_types);
  define_functions(st, prog, &res.owned_types);
  // Typecheck globals init
  for (size_t i = 0; i < prog->as.program.vars.len; ++i) {
    ASTNode *vd = prog->as.program.vars.items[i];
    if (vd->as.var_decl.init) {
      Type *decl = type_from_ast(st, vd->as.var_decl.type);
      Type *init = typecheck_expr(st, &res, vd->as.var_decl.init);
      if (!type_equals(decl, init)) {
        char *ds = type_to_string(decl); char *is = type_to_string(init);
        char buf[256]; snprintf(buf, sizeof(buf), "Type mismatch in var init: %s := %s", ds, is);
        add_error(&res, vd->span, buf);
        free(ds); free(is);
      }
    }
  }
  typecheck_stmt(st, &res, prog->as.program.body);
  // functions
  for (size_t i = 0; i < prog->as.program.functions.len; ++i) {
    symtab_push(st);
    ASTNode *fn = prog->as.program.functions.items[i];
    // params
    for (size_t j = 0; j < fn->as.func_decl.params.len; ++j) {
      ASTParam *pm = &fn->as.func_decl.params.items[j];
      Type *pty = type_from_ast(st, pm->type);
      symtab_define(st, SYM_VAR, pm->name.data, pty);
      typevec_push(&res.temp_types, pty);
    }
    // implicit Result
    if (fn->as.func_decl.result_type) {
      Type *rty = type_from_ast(st, fn->as.func_decl.result_type);
      symtab_define(st, SYM_VAR, "Result", rty);
      typevec_push(&res.temp_types, rty);
    }
    // locals
    if (fn->as.func_decl.locals) {
      for (size_t j = 0; j < fn->as.func_decl.locals->vars.len; ++j) {
        ASTVarDecl *vd = &fn->as.func_decl.locals->vars.items[j];
        Type *lty = type_from_ast(st, vd->type);
        symtab_define(st, SYM_VAR, vd->name.data, lty);
        typevec_push(&res.temp_types, lty);
      }
    }
    typecheck_stmt(st, &res, fn->as.func_decl.body);
    symtab_pop(st);
  }
  symtab_destroy(st);
  return res;
}

static void type_free_temp(Type *t) {
  if (!t) return;
  switch (t->kind) {
  case TYPEK_ARRAY:
    free(t);
    break;
  case TYPEK_TUPLE:
    free(t->as.tuple.items);
    free(t);
    break;
  case TYPEK_OPTIONAL:
    free(t);
    break;
  case TYPEK_RESULT:
    free(t);
    break;
  default:
    type_free(t);
    break;
  }
}

void typecheck_result_free(TypeCheckResult *res) {
  for (size_t i = 0; i < res->errors.len; ++i) free(res->errors.items[i].message);
  free(res->errors.items);
  for (size_t i = 0; i < res->temp_types.len; ++i) {
    if (!typevec_contains(&res->owned_types, res->temp_types.items[i])) type_free_temp(res->temp_types.items[i]);
  }
  free(res->temp_types.items);
  for (size_t i = 0; i < res->owned_types.len; ++i) type_free(res->owned_types.items[i]);
  free(res->owned_types.items);
}
