#ifndef LIMINAL_RUNTIME_H
#define LIMINAL_RUNTIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LVAL_STRING,
  LVAL_ARRAY
} LValueKind;

typedef struct LObject {
  size_t refcount;
  LValueKind kind;
} LObject;

typedef struct LString {
  LObject base;
  size_t len;
  char *data;
} LString;

typedef struct LArray LArray;

typedef struct LValue {
  LValueKind kind;
  union {
    LString *str;
    LArray *arr;
  } as;
} LValue;

struct LArray {
  LObject base;
  size_t len;
  size_t cap;
  LValue *items;
};

// Allocation counters (for tests)
void runtime_reset_counters(void);
size_t runtime_alloc_count(void);
size_t runtime_free_count(void);

// Reference counting
void lobject_retain(LObject *o);
void lobject_release(LObject *o);

// Strings
LString *lstring_new(const char *data, size_t len);
LString *lstring_from_cstr(const char *cstr);

// Arrays
LArray *larray_new(size_t initial_cap);
void larray_push(LArray *arr, LValue v);
LValue larray_get(LArray *arr, size_t idx);
void larray_set(LArray *arr, size_t idx, LValue v);

// LValue helpers
LValue lvalue_string(LString *s);
LValue lvalue_array(LArray *a);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_RUNTIME_H
