#define _POSIX_C_SOURCE 200809L
#include "liminal/symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct Scope {
  Symbol *symbols;
  size_t len;
  size_t cap;
  struct Scope *next;
};

static void *xmalloc(size_t n) { void *p = malloc(n); if (!p) { fprintf(stderr,"OOM\n"); exit(1);} memset(p,0,n); return p; }

Symtab *symtab_create(void) {
  Symtab *st = xmalloc(sizeof(Symtab));
  st->top = NULL;
  symtab_push(st);
  return st;
}

void symtab_destroy(Symtab *st) {
  while (st->top) symtab_pop(st);
  free(st);
}

void symtab_push(Symtab *st) {
  Scope *sc = xmalloc(sizeof(Scope));
  sc->len = 0; sc->cap = 0; sc->symbols = NULL; sc->next = st->top;
  st->top = sc;
}

void symtab_pop(Symtab *st) {
  if (!st->top) return;
  Scope *sc = st->top;
  for (size_t i = 0; i < sc->len; ++i) {
    free(sc->symbols[i].name);
    // types not owned
  }
  free(sc->symbols);
  st->top = sc->next;
  free(sc);
}

int symtab_define(Symtab *st, SymbolKind kind, const char *name, Type *type) {
  // no duplicate check in current scope
  Scope *sc = st->top;
  if (sc->len == sc->cap) {
    sc->cap = sc->cap ? sc->cap * 2 : 4;
    sc->symbols = realloc(sc->symbols, sc->cap * sizeof(Symbol));
  }
  sc->symbols[sc->len].kind = kind;
  sc->symbols[sc->len].name = strdup(name);
  sc->symbols[sc->len].type = type;
  sc->len++;
  return 1;
}

Symbol *symtab_lookup(Symtab *st, const char *name) {
  if (!name) return NULL;
  for (Scope *sc = st->top; sc; sc = sc->next) {
    for (size_t i = 0; i < sc->len; ++i) {
      if (getenv("LIMINAL_DEBUG_TC")) fprintf(stderr, "[symtab] check %s vs %s\n", sc->symbols[i].name, name);
      if (strcasecmp(sc->symbols[i].name, name) == 0) return &sc->symbols[i];
    }
  }
  return NULL;
}
