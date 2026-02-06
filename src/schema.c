#define _POSIX_C_SOURCE 200809L
#include "liminal/schema.h"
#include "liminal/types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} return p; }

static void append(char **buf, size_t *cap, size_t *len, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int needed = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (*len + needed + 1 > *cap) {
    *cap = (*cap + needed + 64) * 2;
    *buf = realloc(*buf, *cap);
  }
  va_start(ap, fmt);
  vsnprintf(*buf + *len, *cap - *len, fmt, ap);
  va_end(ap);
  *len += needed;
}

static void emit_type_json_inner(char **buf, size_t *cap, size_t *len, const ASTType *t) {
  if (!t) { append(buf, cap, len, "\"type\":\"null\""); return; }
  switch (t->kind) {
  case TYPE_IDENT:
    append(buf, cap, len, "\"type\":\"%.*s\"", (int)t->as.ident.name.len, t->as.ident.name.data);
    break;
  case TYPE_CONSTRAINED:
    append(buf, cap, len, "\"type\":\"%s\"", t->as.constrained_type.base.data);
    if (t->as.constrained_type.has_min) append(buf, cap, len, ",\"minimum\":%g", t->as.constrained_type.min);
    if (t->as.constrained_type.has_max) append(buf, cap, len, ",\"maximum\":%g", t->as.constrained_type.max);
    if (t->as.constrained_type.length_constraint) {
      if (t->as.constrained_type.has_min) append(buf, cap, len, ",\"minLength\":%g", t->as.constrained_type.min);
      if (t->as.constrained_type.has_max) append(buf, cap, len, ",\"maxLength\":%g", t->as.constrained_type.max);
    }
    if (t->as.constrained_type.pattern) append(buf, cap, len, ",\"pattern\":\"%s\"", t->as.constrained_type.pattern);
    break;
  case TYPE_RECORD:
    append(buf, cap, len, "\"type\":\"object\",\"properties\":{");
    for (size_t i = 0; i < t->as.record_type.fields.len; ++i) {
      if (i) append(buf, cap, len, ",");
      append(buf, cap, len, "\"%.*s\":{", (int)t->as.record_type.fields.items[i].name.len, t->as.record_type.fields.items[i].name.data);
      emit_type_json_inner(buf, cap, len, t->as.record_type.fields.items[i].type);
      append(buf, cap, len, "}");
    }
    append(buf, cap, len, "}");
    break;
  default:
    append(buf, cap, len, "\"type\":\"unknown\"");
  }
}


char *schema_to_json(const ASTType *schema_type) {
  size_t cap = 1024, len = 0;
  char *buf = xmalloc(cap);
  buf[0] = '\0';
  append(&buf, &cap, &len, "{\"type\":\"object\",\"properties\":{");
  for (size_t i = 0; i < schema_type->as.schema_type.fields.len; ++i) {
    if (i) append(&buf, &cap, &len, ",");
    const ASTFieldDecl *fd = &schema_type->as.schema_type.fields.items[i];
    append(&buf, &cap, &len, "\"%.*s\":{", (int)fd->name.len, fd->name.data);
    emit_type_json_inner(&buf, &cap, &len, fd->type);
    if (fd->describe.len) append(&buf, &cap, &len, ",\"description\":\"%.*s\"", (int)fd->describe.len, fd->describe.data);
    append(&buf, &cap, &len, "}");
  }
  append(&buf, &cap, &len, "},\"required\":[");
  for (size_t i = 0; i < schema_type->as.schema_type.fields.len; ++i) {
    if (i) append(&buf, &cap, &len, ",");
    const ASTFieldDecl *fd = &schema_type->as.schema_type.fields.items[i];
    append(&buf, &cap, &len, "\"%.*s\"", (int)fd->name.len, fd->name.data);
  }
  append(&buf, &cap, &len, "]}");
  return buf;
}
