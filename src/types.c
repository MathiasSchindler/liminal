#define _POSIX_C_SOURCE 200809L
#include "liminal/types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} memset(p,0,n); return p; }

static Type singleton_int = {.kind = TYPEK_INT};
static Type singleton_real = {.kind = TYPEK_REAL};
static Type singleton_bool = {.kind = TYPEK_BOOL};
static Type singleton_string = {.kind = TYPEK_STRING};
static Type singleton_bytes = {.kind = TYPEK_BYTES};
static Type singleton_char = {.kind = TYPEK_CHAR};
static Type singleton_byte = {.kind = TYPEK_BYTE};
static Type singleton_unknown = {.kind = TYPEK_UNKNOWN};
static Type singleton_optional_unknown = {.kind = TYPEK_OPTIONAL, .as.optional.inner = &singleton_unknown};

Type *type_primitive(TypeKindSem k) {
  switch (k) {
  case TYPEK_INT: return &singleton_int;
  case TYPEK_REAL: return &singleton_real;
  case TYPEK_BOOL: return &singleton_bool;
  case TYPEK_STRING: return &singleton_string;
  case TYPEK_BYTES: return &singleton_bytes;
  case TYPEK_CHAR: return &singleton_char;
  case TYPEK_BYTE: return &singleton_byte;
  case TYPEK_UNKNOWN: default: return &singleton_unknown;
  }
}

Type *type_array(Type *elem) {
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_ARRAY;
  t->as.array.elem = elem;
  return t;
}

Type *type_tuple(TypeVec elements) {
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_TUPLE;
  t->as.tuple.items = elements.items;
  t->as.tuple.len = elements.len;
  t->as.tuple.cap = elements.cap;
  return t;
}

Type *type_optional(Type *inner) {
  if (inner == &singleton_unknown) return &singleton_optional_unknown;
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_OPTIONAL;
  t->as.optional.inner = inner;
  return t;
}

Type *type_result(Type *ok, Type *err) {
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_RESULT;
  t->as.result.ok = ok;
  t->as.result.err = err;
  return t;
}

Type *type_alias(const char *name, Type *target) {
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_ALIAS;
  t->as.alias.name = strdup(name);
  t->as.alias.target = target;
  return t;
}

Type *type_schema(const char *name) {
  Type *t = xmalloc(sizeof(Type));
  t->kind = TYPEK_SCHEMA;
  t->as.schema.name = name ? strdup(name) : NULL;
  t->as.schema.items = NULL;
  t->as.schema.len = 0;
  t->as.schema.cap = 0;
  return t;
}

void schema_add_field(Type *schema_type, const char *name, Type *field_type) {
  if (!schema_type || (schema_type->kind != TYPEK_SCHEMA && schema_type->kind != TYPEK_RECORD && schema_type->kind != TYPEK_ENUM)) return;
  SchemaField *fields = schema_type->as.schema.items;
  size_t len = schema_type->as.schema.len;
  size_t cap = schema_type->as.schema.cap;
  if (len == cap) {
    cap = cap ? cap * 2 : 4;
    fields = realloc(fields, cap * sizeof(SchemaField));
    schema_type->as.schema.items = fields;
    schema_type->as.schema.cap = cap;
  }
  fields[len].name = strdup(name);
  fields[len].type = field_type;
  schema_type->as.schema.len = len + 1;
}

int typevec_contains(TypeVec *vec, Type *t) {
  for (size_t i=0;i<vec->len;++i) if (vec->items[i]==t) return 1;
  return 0;
}

void typevec_push(TypeVec *vec, Type *t) {
  if (vec->len == vec->cap) {
    vec->cap = vec->cap ? vec->cap * 2 : 4;
    vec->items = realloc(vec->items, vec->cap * sizeof(Type *));
  }
  vec->items[vec->len++] = t;
}

SchemaField *schema_find_field(Type *schema_type, const char *name) {
  if (!schema_type || (schema_type->kind != TYPEK_SCHEMA && schema_type->kind != TYPEK_RECORD && schema_type->kind != TYPEK_ENUM)) return NULL;
  if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] schema_find_field kind=%d len=%zu lookup=%s\n", schema_type->kind, schema_type->as.schema.len, name);
  for (size_t i = 0; i < schema_type->as.schema.len; ++i) {
    if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr,"[tc] field[%zu]=%s\n", i, schema_type->as.schema.items[i].name);
    if (strcmp(schema_type->as.schema.items[i].name, name) == 0) return &schema_type->as.schema.items[i];
  }
  return NULL;
}

static int type_equals_internal(const Type *a, const Type *b) {
  if (a == b) return 1;
  if (!a || !b) return 0;
  if (a->kind == TYPEK_ALIAS) return type_equals_internal(a->as.alias.target, b);
  if (b->kind == TYPEK_ALIAS) return type_equals_internal(a, b->as.alias.target);
  if ((a->kind == TYPEK_ENUM && b->kind == TYPEK_INT) || (a->kind == TYPEK_INT && b->kind == TYPEK_ENUM)) return 1;
  if (a->kind != b->kind) return 0;
  switch (a->kind) {
  case TYPEK_OPTIONAL:
    return type_equals_internal(a->as.optional.inner, b->as.optional.inner);
  case TYPEK_RESULT:
    return type_equals_internal(a->as.result.ok, b->as.result.ok) && type_equals_internal(a->as.result.err, b->as.result.err);
  case TYPEK_ARRAY:
    return type_equals_internal(a->as.array.elem, b->as.array.elem);
  case TYPEK_TUPLE:
    if (a->as.tuple.len != b->as.tuple.len) return 0;
    for (size_t i = 0; i < a->as.tuple.len; ++i) {
      if (!type_equals_internal(a->as.tuple.items[i], b->as.tuple.items[i])) return 0;
    }
    return 1;
  case TYPEK_SCHEMA:
  case TYPEK_RECORD:
  case TYPEK_ENUM:
    if (a->as.schema.len != b->as.schema.len) return 0;
    for (size_t i = 0; i < a->as.schema.len; ++i) {
      SchemaField *fa = &a->as.schema.items[i];
      SchemaField *fb = schema_find_field((Type *)b, fa->name);
      if (!fb) return 0;
      if (!type_equals_internal(fa->type, fb->type)) return 0;
    }
    return 1;
  default:
    return 1;
  }
}

int type_equals(const Type *a, const Type *b) { return type_equals_internal(a, b); }

const char *typekind_name(TypeKindSem k) {
  switch (k) {
  case TYPEK_INT: return "Integer";
  case TYPEK_REAL: return "Real";
  case TYPEK_BOOL: return "Boolean";
  case TYPEK_STRING: return "String";
  case TYPEK_BYTES: return "Bytes";
  case TYPEK_CHAR: return "Char";
  case TYPEK_BYTE: return "Byte";
  case TYPEK_ARRAY: return "Array";
  case TYPEK_TUPLE: return "Tuple";
  case TYPEK_ALIAS: return "Alias";
  case TYPEK_SCHEMA: return "Schema";
  case TYPEK_RECORD: return "Record";
  case TYPEK_ENUM: return "Enum";
  case TYPEK_OPTIONAL: return "Optional";
  case TYPEK_RESULT: return "Result";
  case TYPEK_UNKNOWN: return "Unknown";
  }
  return "Unknown";
}

char *type_to_string(const Type *t) {
  if (!t) return strdup("<null>");
  if (t->kind == TYPEK_ALIAS && t->as.alias.name) return strdup(t->as.alias.name);
  const char *base = typekind_name(t->kind);
  if (t->kind == TYPEK_OPTIONAL) {
    char *inner = type_to_string(t->as.optional.inner);
    size_t n = strlen(inner) + 2;
    char *buf = malloc(n);
    snprintf(buf, n, "?%s", inner);
    free(inner);
    return buf;
  }
  if (t->kind == TYPEK_RESULT) {
    char *ok = type_to_string(t->as.result.ok);
    size_t n = strlen(ok) + 2;
    char *buf = malloc(n);
    snprintf(buf, n, "!%s", ok);
    free(ok);
    return buf;
  }
  if (t->kind == TYPEK_ARRAY) {
    char *elem = type_to_string(t->as.array.elem);
    size_t n = strlen(elem) + 8;
    char *buf = malloc(n);
    snprintf(buf, n, "array(%s)", elem);
    free(elem);
    return buf;
  }
  if (t->kind == TYPEK_TUPLE) {
    // emit tuple<a,b>
    size_t cap = 64; char *buf = malloc(cap); buf[0] = '\0';
    strcat(buf, "tuple<");
    for (size_t i = 0; i < t->as.tuple.len; ++i) {
      char *s = type_to_string(t->as.tuple.items[i]);
      size_t need = strlen(buf) + strlen(s) + 4;
      if (need > cap) { cap = need * 2; buf = realloc(buf, cap); }
      strcat(buf, s);
      free(s);
      if (i + 1 < t->as.tuple.len) strcat(buf, ",");
    }
    strcat(buf, ">" );
    return buf;
  }
  if (t->kind == TYPEK_SCHEMA) {
    return strdup("Schema");
  }
  return strdup(base);
}

void type_free(Type *t) {
  if (!t) return;
  // primitives are static singletons
  if (t == &singleton_int || t == &singleton_real || t == &singleton_bool ||
      t == &singleton_string || t == &singleton_bytes || t == &singleton_char ||
      t == &singleton_byte || t == &singleton_unknown) return;
  switch (t->kind) {
  case TYPEK_ARRAY:
    /* element types are not owned here */
    break;
  case TYPEK_TUPLE:
    for (size_t i = 0; i < t->as.tuple.len; ++i) type_free(t->as.tuple.items[i]);
    free(t->as.tuple.items);
    break;
  case TYPEK_ALIAS:
    free(t->as.alias.name);
    // target not owned
    break;
  case TYPEK_OPTIONAL:
    // inner not owned
    break;
  case TYPEK_RESULT:
    // inner not owned
    break;
  case TYPEK_SCHEMA:
  case TYPEK_RECORD:
  case TYPEK_ENUM:
    if (t->as.schema.name) free(t->as.schema.name);
    for (size_t i = 0; i < t->as.schema.len; ++i) {
      if (t->as.schema.items[i].name) free(t->as.schema.items[i].name);
      // field types not owned
    }
    free(t->as.schema.items);
    break;
  default:
    break;
  }
  free(t);
}
