#ifndef LIMINAL_IR_H
#define LIMINAL_IR_H

#include <stddef.h>
#include "liminal/ast.h"
#include "liminal/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  IR_NOP,
  IR_CONST_INT,
  IR_CONST_REAL,
  IR_CONST_STRING,
  IR_LOAD_VAR,
  IR_STORE_VAR,
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,
  IR_MOD,
  IR_EQ,
  IR_NEQ,
  IR_LT,
  IR_GT,
  IR_LE,
  IR_GE,
  IR_JUMP,
  IR_JUMP_IF_FALSE,
  IR_LABEL,
  IR_RET,
  IR_PRINT,
  IR_PRINTLN,
  IR_READLN,
  IR_READ_FILE,
  IR_WRITE_FILE,
  IR_ASK,
  IR_RESULT_UNWRAP,
  IR_RESULT_IS_OK,
  IR_RESULT_UNWRAP_ERR,
  IR_MAKE_RESULT_OK,
  IR_MAKE_RESULT_ERR,
  IR_CONCAT,
  IR_RESULT_OR_FALLBACK,
  IR_CALL,
  IR_AND,
  IR_OR,
  IR_CONST_BOOL,
  IR_CONST_OPTIONAL_NONE,
  IR_INDEX
} IrOp;

typedef struct {
  IrOp op;
  int dest;
  int arg1;
  int arg2;
  double f; // for reals / flags
  char *s; // for strings/var names/labels/oracle name
  char *s2; // auxiliary string (schema type name)
} IrInstr;

typedef struct {
  IrInstr *items;
  size_t len;
  size_t cap;
} IrInstrVec;

typedef struct {
  char *name;
  char **params;
  int param_count;
  IrInstrVec instrs;
  int next_temp;
  int next_label;
} IrFunc;

typedef struct {
  IrFunc *items;
  size_t len;
  size_t cap;
} IrFuncVec;

typedef struct {
  IrFuncVec funcs;
  TypeVec schemas; // schema metadata
} IrProgram;

// Core
IrProgram *ir_program_new(void);
void ir_program_add_func(IrProgram *prog, IrFunc func);
void ir_program_free(IrProgram *prog);
char *ir_program_print(const IrProgram *prog);

// Builder API
IrFunc ir_func_create(const char *name);
int ir_emit_const_int(IrFunc *f, int value);
int ir_emit_const_real(IrFunc *f, double value);
int ir_emit_const_string(IrFunc *f, const char *s);
int ir_emit_const_bool(IrFunc *f, int value);
int ir_emit_binop(IrFunc *f, IrOp op, int lhs, int rhs);
int ir_emit_load_var(IrFunc *f, const char *name);
void ir_emit_store_var(IrFunc *f, const char *name, int src_temp);
void ir_emit_jump(IrFunc *f, const char *label);
void ir_emit_jump_if_false(IrFunc *f, int cond_temp, const char *label);
void ir_emit_label(IrFunc *f, const char *label);
void ir_emit_ret(IrFunc *f, int temp);
void ir_emit_print(IrFunc *f, int temp, int newline);
void ir_emit_readln(IrFunc *f, const char *name);
int ir_emit_read_file(IrFunc *f, int path_temp);
void ir_emit_write_file(IrFunc *f, int path_temp, int content_temp);
int ir_emit_ask(IrFunc *f, int prompt_temp, int fallback_temp, const char *oracle_name, const char *schema_name);
int ir_emit_result_unwrap(IrFunc *f, int result_temp, int fallback_temp);
int ir_emit_result_is_ok(IrFunc *f, int result_temp);
int ir_emit_concat(IrFunc *f, int a_temp, int b_temp);
int ir_emit_result_or_fallback(IrFunc *f, int result_temp, int fallback_temp);
int ir_emit_call(IrFunc *f, const char *fname, int arg0_temp, int arg1_temp);

// Validator
int ir_validate(const IrProgram *prog, char **errmsg);

// Translator
IrProgram *ir_from_ast(const ASTNode *prog);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_IR_H
