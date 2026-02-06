#ifndef LIMINAL_TYPES_H
#define LIMINAL_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  TYPEK_INT,
  TYPEK_REAL,
  TYPEK_BOOL,
  TYPEK_STRING,
  TYPEK_BYTES,
  TYPEK_CHAR,
  TYPEK_BYTE,
  TYPEK_ARRAY,
  TYPEK_TUPLE,
  TYPEK_ALIAS,
  TYPEK_SCHEMA,
  TYPEK_RECORD,
  TYPEK_ENUM,
  TYPEK_OPTIONAL,
  TYPEK_RESULT,
  TYPEK_UNKNOWN
} TypeKindSem; // semantic type kind

typedef struct Type Type;

typedef struct {
  Type **items;
  size_t len;
  size_t cap;
} TypeVec;

typedef struct {
  Type *elem;
} ArrayType;

typedef struct {
  Type **items;
  size_t len;
  size_t cap;
} TupleType;

typedef struct {
  char *name;
  Type *target; // alias target
} AliasType;

typedef struct {
  char *name;
  Type *type;
} SchemaField;

typedef struct {
  char *name;
  SchemaField *items;
  size_t len;
  size_t cap;
} SchemaType;

typedef struct {
  Type *inner;
} OptionalType;

typedef struct {
  Type *ok;
  Type *err;
} ResultType;

typedef struct Type {
  TypeKindSem kind;
  union {
    ArrayType array;
    TupleType tuple;
    AliasType alias;
    SchemaType schema;
    OptionalType optional;
    ResultType result;
  } as;
} Type;

Type *type_result(Type *ok, Type *err);

Type *type_primitive(TypeKindSem k);
Type *type_array(Type *elem);
Type *type_tuple(TypeVec elements);
Type *type_alias(const char *name, Type *target);
Type *type_schema(const char *name);
Type *type_optional(Type *inner);
void schema_add_field(Type *schema_type, const char *name, Type *field_type);

int type_equals(const Type *a, const Type *b);
const char *typekind_name(TypeKindSem k);
char *type_to_string(const Type *t);
void type_free(Type *t);
SchemaField *schema_find_field(Type *schema_type, const char *name);
int typevec_contains(TypeVec *vec, Type *t);
void typevec_push(TypeVec *vec, Type *t);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_TYPES_H
