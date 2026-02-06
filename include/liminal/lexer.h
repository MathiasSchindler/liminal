#ifndef LIMINAL_LEXER_H
#define LIMINAL_LEXER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int line;    /* 1-based */
  int column;  /* 1-based, Unicode codepoints approximated as bytes for now */
  int offset;  /* 0-based byte offset */
  int length;  /* bytes */
} LiminalSpan;

typedef enum {
  TK_EOF = 0,
  TK_IDENTIFIER,
  TK_KEYWORD,
  TK_INTEGER,
  TK_REAL,
  TK_STRING,
  TK_BYTES,
  TK_CHAR,
  TK_FSTRING,
  TK_DURATION,
  TK_MONEY,
  TK_BOOL,

  TK_COLON,
  TK_SEMICOLON,
  TK_COMMA,
  TK_DOT,
  TK_DOT_DOT,
  TK_LPAREN,
  TK_RPAREN,
  TK_LBRACKET,
  TK_RBRACKET,
  TK_LBRACE,
  TK_RBRACE,

  TK_PLUS,
  TK_MINUS,
  TK_STAR,
  TK_SLASH,
  TK_DIV,
  TK_MOD,
  TK_ASSIGN,      /* := */
  TK_EQ,          /* = */
  TK_NEQ,         /* <> */
  TK_LT,
  TK_GT,
  TK_LE,
  TK_GE,
  TK_AND,
  TK_OR,
  TK_NOT,
  TK_QMARK,
  TK_BANG,

  TK_ERROR
} TokenKind;

typedef struct {
  TokenKind kind;
  LiminalSpan span;
  const char *lexeme; /* points into original source */
  size_t lexeme_len;
  const char *keyword; /* non-NULL when TK_KEYWORD */
  char error_msg[128]; /* when TK_ERROR */
} Token;

typedef struct Lexer Lexer;

Lexer *lexer_create(const char *input, size_t length);
void lexer_destroy(Lexer *lexer);
Token lexer_next(Lexer *lexer);
const char *token_kind_name(TokenKind kind);
int is_keyword(const char *ident, size_t len);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_LEXER_H
