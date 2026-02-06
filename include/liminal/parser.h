#ifndef LIMINAL_PARSER_H
#define LIMINAL_PARSER_H

#include "liminal/ast.h"
#include "liminal/lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Parser Parser;

typedef struct {
  char *message;
  LiminalSpan span;
} ParseError;

typedef struct {
  ParseError *items;
  size_t len;
  size_t cap;
} ParseErrorVec;

Parser *parser_create(const char *src, size_t len);
void parser_destroy(Parser *p);
ASTNode *parse_program(Parser *p);
int parser_has_errors(Parser *p);
ParseErrorVec parser_errors(Parser *p);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_PARSER_H
