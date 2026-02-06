#define _POSIX_C_SOURCE 200809L
#include "liminal/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t g_allocs = 0;
static size_t g_frees = 0;

void runtime_reset_counters(void) { g_allocs = 0; g_frees = 0; }
size_t runtime_alloc_count(void) { return g_allocs; }
size_t runtime_free_count(void) { return g_frees; }

static void *xmalloc(size_t n) {
  void *p = malloc(n);
  if (!p) { fprintf(stderr, "OOM\n"); exit(1); }
  memset(p, 0, n);
  g_allocs++;
  return p;
}

static void xfree(void *p) {
  if (!p) return;
  g_frees++;
  free(p);
}

static void lvalue_retain(LValue v);
static void lvalue_release(LValue v);

void lobject_retain(LObject *o) {
  if (!o) return;
  o->refcount++;
}

void lobject_release(LObject *o) {
  if (!o) return;
  if (--o->refcount == 0) {
    switch (o->kind) {
    case LVAL_STRING: {
      LString *s = (LString *)o;
      xfree(s->data);
      xfree(s);
      break;
    }
    case LVAL_ARRAY: {
      LArray *a = (LArray *)o;
      for (size_t i = 0; i < a->len; ++i) lvalue_release(a->items[i]);
      xfree(a->items);
      xfree(a);
      break;
    }
    }
  }
}

LString *lstring_new(const char *data, size_t len) {
  LString *s = (LString *)xmalloc(sizeof(LString));
  s->base.kind = LVAL_STRING;
  s->base.refcount = 1;
  s->len = len;
  s->data = (char *)xmalloc(len + 1);
  memcpy(s->data, data, len);
  s->data[len] = '\0';
  return s;
}

LString *lstring_from_cstr(const char *cstr) {
  return lstring_new(cstr, strlen(cstr));
}

LArray *larray_new(size_t initial_cap) {
  LArray *a = (LArray *)xmalloc(sizeof(LArray));
  a->base.kind = LVAL_ARRAY;
  a->base.refcount = 1;
  a->len = 0;
  a->cap = initial_cap ? initial_cap : 4;
  a->items = (LValue *)xmalloc(a->cap * sizeof(LValue));
  return a;
}

static void ensure_cap(LArray *a, size_t need) {
  if (need <= a->cap) return;
  size_t newcap = a->cap * 2;
  if (newcap < need) newcap = need;
  a->items = realloc(a->items, newcap * sizeof(LValue));
  a->cap = newcap;
}

void larray_push(LArray *a, LValue v) {
  ensure_cap(a, a->len + 1);
  lvalue_retain(v);
  a->items[a->len++] = v;
}

LValue larray_get(LArray *a, size_t idx) {
  if (idx >= a->len) {
    LValue v = { .kind = LVAL_STRING, .as.str = NULL };
    return v;
  }
  return a->items[idx];
}

void larray_set(LArray *a, size_t idx, LValue v) {
  if (idx >= a->len) return;
  lvalue_release(a->items[idx]);
  lvalue_retain(v);
  a->items[idx] = v;
}

LValue lvalue_string(LString *s) {
  LValue v; v.kind = LVAL_STRING; v.as.str = s; return v;
}

LValue lvalue_array(LArray *a) {
  LValue v; v.kind = LVAL_ARRAY; v.as.arr = a; return v;
}

static void lvalue_retain(LValue v) {
  switch (v.kind) {
  case LVAL_STRING: lobject_retain((LObject *)v.as.str); break;
  case LVAL_ARRAY: lobject_retain((LObject *)v.as.arr); break;
  }
}

static void lvalue_release(LValue v) {
  switch (v.kind) {
  case LVAL_STRING: lobject_release((LObject *)v.as.str); break;
  case LVAL_ARRAY: lobject_release((LObject *)v.as.arr); break;
  }
}
