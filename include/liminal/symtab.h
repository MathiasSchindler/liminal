#ifndef LIMINAL_SYMTAB_H
#define LIMINAL_SYMTAB_H

#include "liminal/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { SYM_VAR, SYM_FUNC, SYM_TYPE } SymbolKind;

typedef struct Symbol {
  SymbolKind kind;
  char *name;
  Type *type; // for types: alias target
} Symbol;

typedef struct Scope Scope;

typedef struct Symtab {
  Scope *top;
} Symtab;

Symtab *symtab_create(void);
void symtab_destroy(Symtab *st);
void symtab_push(Symtab *st);
void symtab_pop(Symtab *st);
int symtab_define(Symtab *st, SymbolKind kind, const char *name, Type *type);
Symbol *symtab_lookup(Symtab *st, const char *name);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_SYMTAB_H
