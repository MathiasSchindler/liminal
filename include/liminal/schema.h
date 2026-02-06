#ifndef LIMINAL_SCHEMA_H
#define LIMINAL_SCHEMA_H

#include "liminal/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Emit JSON schema string for a TYPE_SCHEMA ASTType
char *schema_to_json(const ASTType *schema_type);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_SCHEMA_H
