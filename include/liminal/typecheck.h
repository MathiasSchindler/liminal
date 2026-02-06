#ifndef LIMINAL_TYPECHECK_H
#define LIMINAL_TYPECHECK_H

#include "liminal/ast.h"
#include "liminal/types.h"
#include "liminal/symtab.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char *message;
  LiminalSpan span;
} TypeCheckError;

typedef struct {
  TypeCheckError *items;
  size_t len;
  size_t cap;
} TypeCheckErrorVec;

typedef struct {
  int ok;
  TypeCheckErrorVec errors;
  TypeVec temp_types;
  TypeVec owned_types;
} TypeCheckResult;

TypeCheckResult typecheck_program(ASTNode *prog);
void typecheck_result_free(TypeCheckResult *res);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_TYPECHECK_H
