#ifndef LIMINAL_AST_H
#define LIMINAL_AST_H

#include "liminal/lexer.h"
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ASTNode ASTNode;
typedef struct ASTExpr ASTExpr;
typedef struct ASTStmt ASTStmt;
typedef struct ASTType ASTType;

typedef struct {
  ASTNode **items;
  size_t len;
  size_t cap;
} ASTNodeVec;

typedef struct {
  ASTExpr **items;
  size_t len;
  size_t cap;
} ASTExprVec;

typedef struct {
  ASTStmt **items;
  size_t len;
  size_t cap;
} ASTStmtVec;

typedef struct {
  ASTType **items;
  size_t len;
  size_t cap;
} ASTTypeVec;

typedef struct {
  char *data;
  size_t len;
} String;

typedef struct {
  String key;
  ASTExpr *value;
} ASTField;

typedef struct {
  ASTField *items;
  size_t len;
  size_t cap;
} ASTFieldVec;

typedef enum {
  AST_PROGRAM,
  AST_USE_CLAUSE,
  AST_CONFIG_ITEM,
  AST_TYPE_DECL,
  AST_SCHEMA_DECL,
  AST_ORACLE_DECL,
  AST_VAR_DECL,
  AST_FUNC_DECL,
  AST_BLOCK,
  AST_STMT,
  AST_EXPR,
  AST_TYPE
} ASTKind;

typedef enum {
  EXPR_LITERAL,
  EXPR_IDENT,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_CALL,
  EXPR_INDEX,
  EXPR_SLICE,
  EXPR_FIELD,
  EXPR_TUPLE,
  EXPR_ARRAY,
  EXPR_RECORD,
  EXPR_ASK,
  EXPR_CONSULT,
  EXPR_EMBED,
  EXPR_CONTEXT,
  EXPR_CONCAT
} ExprKind;

typedef enum {
  STMT_ASSIGN,
  STMT_IF,
  STMT_WHILE,
  STMT_FOR,
  STMT_REPEAT,
  STMT_CASE,
  STMT_LOOP,
  STMT_PARALLEL,
  STMT_BREAK,
  STMT_CONTINUE,
  STMT_RETURN,
  STMT_TRY,
  STMT_BLOCK,
  STMT_FOR_IN,
  STMT_EXPR
} StmtKind;

typedef enum {
  TYPE_PRIMITIVE,
  TYPE_IDENT,
  TYPE_ARRAY,
  TYPE_TUPLE,
  TYPE_RECORD,
  TYPE_ENUM,
  TYPE_OPTIONAL,
  TYPE_RESULT,
  TYPE_CONSTRAINED,
  TYPE_SCHEMA
} TypeKind;

typedef struct {
  String name;
} ASTIdent;

typedef struct {
  String value;
  TokenKind literal_kind; // INTEGER, REAL, STRING, etc.
} ASTLiteral;

typedef struct {
  int op; // token kind or custom enum
  ASTExpr *expr;
} ASTUnary;

typedef struct {
  int op; // token kind
  ASTExpr *lhs;
  ASTExpr *rhs;
} ASTBinary;

typedef struct {
  ASTExpr *callee;
  ASTExprVec args;
} ASTCall;

typedef struct {
  ASTExpr *base;
  ASTExprVec indices;
} ASTIndex;

typedef struct {
  ASTExpr *base;
  ASTExpr *lo; // nullable
  ASTExpr *hi; // nullable
} ASTSlice;

typedef struct {
  ASTExpr *base;
  String field;
} ASTFieldAccess;

typedef struct {
  ASTExprVec elements;
} ASTTuple;

typedef struct {
  ASTExprVec elements;
} ASTArray;

typedef struct {
  ASTFieldVec fields;
} ASTRecord;

typedef struct {
  ASTExpr *oracle;
  ASTExpr *input;
  ASTType *into_type; // nullable
  ASTExpr *timeout;   // nullable (duration literal)
  ASTExpr *fallback;  // nullable
  int with_cost;      // bool
} ASTAsk;

typedef struct {
  ASTExpr *oracle;
  ASTExpr *input;
} ASTEmbed;

typedef struct {
  ASTExpr *oracle;
  ASTExpr *input;
  ASTType *into_type; // nullable
  int attempts; // default 1
  ASTExpr *hint; // optional
  ASTExpr *fallback; // optional
} ASTConsult;

typedef struct {
  ASTExpr *ctx;
  ASTExprVec methods; // simplified; we keep expressions of method calls
} ASTContext;

typedef struct {
  String name;
  ASTType *type;
} ASTParam;

typedef struct {
  ASTParam *items;
  size_t len;
  size_t cap;
} ASTParamVec;

typedef struct {
  String name;
  ASTType *type;
} ASTVar;

typedef struct {
  ASTVar *items;
  size_t len;
  size_t cap;
} ASTVarVec;

typedef struct {
  String name;
  ASTType *type;
  ASTExpr *init; // nullable
} ASTVarDecl;

typedef struct {
  ASTVarDecl *items;
  size_t len;
  size_t cap;
} ASTVarDeclVec;

typedef struct {
  String name;
  ASTType *type;
  String describe;
} ASTFieldDecl;

typedef struct {
  ASTFieldDecl *items;
  size_t len;
  size_t cap;
} ASTFieldDeclVec;

typedef struct {
  String name;
  ASTType *type;
  ASTExpr *value;
} ASTConfigItem;

typedef struct {
  String name;
  ASTType *type;
} ASTTypeDecl;

typedef struct {
  String name;
  ASTFieldDeclVec fields;
} ASTRecordType;

typedef struct {
  ASTType *elem;
  int is_fixed; // if fixed size array
  ASTExprVec dims; // for fixed dims
  ASTExpr *len_constraint; // nullable
  int has_len_min;
  int has_len_max;
  double len_min;
  double len_max;
} ASTArrayType;

typedef struct {
  ASTTypeVec elements;
} ASTTupleType;

typedef struct {
  String *items;
  size_t len;
  size_t cap;
} ASTEnumType;

typedef struct {
  ASTType *option;
} ASTOptionalType;

typedef struct {
  ASTType *ok;
  ASTType *err; // nullable (with clause)
} ASTResultType;

typedef struct {
  String base;
  double min;
  double max;
  int has_min;
  int has_max;
  int length_constraint;
  char *pattern; // malloced
} ASTConstrainedType;

typedef struct {
  String name;
  ASTFieldDeclVec fields;
} ASTSchemaType;

typedef struct ASTType {
  TypeKind kind;
  LiminalSpan span;
  union {
    String primitive;
    ASTIdent ident;
    ASTArrayType array_type;
    ASTTupleType tuple_type;
    ASTRecordType record_type;
    ASTEnumType enum_type;
    ASTOptionalType optional_type;
    ASTResultType result_type;
    ASTConstrainedType constrained_type;
    ASTSchemaType schema_type;
  } as;
} ASTType;

typedef struct ASTExpr {
  ExprKind kind;
  LiminalSpan span;
  union {
    ASTLiteral literal;
    ASTIdent ident;
    ASTUnary unary;
    ASTBinary binary;
    ASTCall call;
    ASTIndex index;
    ASTSlice slice;
    ASTFieldAccess field;
    ASTTuple tuple;
    ASTArray array;
    ASTRecord record;
    ASTAsk ask;
    ASTConsult consult;
    ASTEmbed embed;
    ASTContext context;
    struct { ASTExpr *lhs; ASTExpr *rhs; } concat;
  } as;
} ASTExpr;

typedef struct ASTStmt {
  StmtKind kind;
  LiminalSpan span;
  union {
    struct { ASTExpr *target; ASTExpr *value; } assign;
    struct { ASTExpr *cond; ASTStmt *then_branch; ASTStmt *else_branch; } if_stmt;
    struct { ASTExpr *cond; ASTStmt *body; } while_stmt;
    struct { ASTExpr *init; ASTExpr *to; ASTStmt *body; ASTIdent var; int descending; } for_stmt;
    struct { ASTIdent var; ASTExpr *iterable; ASTStmt *body; } for_in_stmt;
    struct { ASTStmt *body; ASTExpr *cond; } repeat_stmt;
    struct { ASTExpr *expr; ASTStmtVec branches; ASTExprVec patterns; ASTStmt *else_branch; } case_stmt;
    struct { ASTStmtVec body; } loop_stmt;
    struct { ASTStmtVec body; } parallel_stmt;
    struct { ASTExpr *value; } return_stmt;
    struct { ASTStmt *try_block; ASTStmt *except_block; } try_stmt;
    struct { ASTStmtVec stmts; } block;
    struct { ASTExpr *expr; } expr_stmt;
  } as;
} ASTStmt;

typedef struct {
  ASTVarDeclVec vars;
} ASTVarBlock;

typedef struct {
  String name;
  ASTParamVec params;
  ASTType *result_type;
  ASTVarBlock *locals; // nullable
  ASTStmt *body;
} ASTFunction;

typedef struct {
  String name;
  ASTType *type;
  String provider;
} ASTOracleDecl;

typedef struct ASTNode {
  ASTKind kind;
  LiminalSpan span;
  union {
    struct {
      String name;
      ASTNodeVec uses;
      ASTNodeVec config_items;
      ASTNodeVec types;
      ASTNodeVec oracles;
      ASTNodeVec vars;
      ASTNodeVec functions;
      ASTStmt *body;
    } program;
    ASTConfigItem config;
    ASTTypeDecl type_decl;
    ASTOracleDecl oracle_decl;
    ASTVarDecl var_decl;
    ASTFunction func_decl;
  } as;
} ASTNode;

ASTNode *ast_program_new(String name);
ASTExpr *ast_make_literal(Token tk);
ASTExpr *ast_make_ident(Token tk);
void ast_node_vec_push(ASTNodeVec *vec, ASTNode *node);
void ast_expr_vec_push(ASTExprVec *vec, ASTExpr *expr);
void ast_stmt_vec_push(ASTStmtVec *vec, ASTStmt *stmt);
void ast_type_vec_push(ASTTypeVec *vec, ASTType *type);
void ast_field_vec_push(ASTFieldVec *vec, ASTField field);
void ast_param_vec_push(ASTParamVec *vec, ASTParam param);
void ast_var_decl_vec_push(ASTVarDeclVec *vec, ASTVarDecl decl);
void ast_field_decl_vec_push(ASTFieldDeclVec *vec, ASTFieldDecl decl);
void ast_enum_vec_push(ASTEnumType *vec, String item);

ASTStmt *ast_make_block(ASTStmtVec *stmts, LiminalSpan span);

void ast_free(ASTNode *node);
void ast_type_free(ASTType *type);
ASTType *ast_type_clone(const ASTType *t);
ASTExpr *ast_expr_clone(const ASTExpr *e);

// Debug printer
void ast_print(const ASTNode *node, FILE *out);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_AST_H
