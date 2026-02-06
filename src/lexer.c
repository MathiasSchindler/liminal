#define _POSIX_C_SOURCE 200809L

#include "liminal/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <strings.h>
#endif

struct Lexer {
  const char *src;
  size_t len;
  size_t pos;  /* current offset */
  int line;
  int col;
};

static int is_ident_start(int c) {
  return isalpha(c) || c == '_';
}

static int is_ident_part(int c) {
  return isalnum(c) || c == '_';
}


static int is_keyword_ci(const char *s, size_t len, const char *kw) {
#ifdef __linux__
  size_t kwlen = strlen(kw);
  if (len != kwlen) return 0;
  return strncasecmp(s, kw, len) == 0;
#else
  size_t kwlen = strlen(kw);
  if (len != kwlen) return 0;
  for (size_t i = 0; i < len; ++i) {
    if (tolower((unsigned char)s[i]) != tolower((unsigned char)kw[i])) return 0;
  }
  return 1;
#endif
}

static const char *KEYWORDS[] = {
    "program",   "uses",      "config",    "types",     "oracles",   "var",
    "function",  "begin",     "end",       "record",    "schema",    "describe",
    "matching",  "array",     "of",        "tuple",     "integer",   "real",
    "boolean",   "char",      "string",    "byte",      "bytes",     "true",
    "false",     "if",        "then",      "else",      "case",      "for",
    "to",        "do",        "in",        "while",     "repeat",    "until",
    "loop",      "parallel",  "break",     "continue",  "return",    "result",
    "try",       "except",    "on",        "consult",   "within",    "budget",
    "stream",    "retry",     "yield",     "wait",      "attempts",  "timeout",
    "from",      "failure",   "hint",      "into",      "with",      "cost",
    "context",   "create",    "withsystem",
    "withmaxtokens", "onoverflow", "strategy", "ask", "embed"};

int is_keyword(const char *ident, size_t len) {
  for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++i) {
    if (is_keyword_ci(ident, len, KEYWORDS[i])) return 1;
  }
  return 0;
}

static int ident_to_token_kind(const char *ident, size_t len, TokenKind *out_kind,
                               const char **out_kw) {
  if (is_keyword_ci(ident, len, "div")) {
    *out_kind = TK_DIV; return 1;
  }
  if (is_keyword_ci(ident, len, "mod")) {
    *out_kind = TK_MOD; return 1;
  }
  if (is_keyword_ci(ident, len, "and")) {
    *out_kind = TK_AND; return 1;
  }
  if (is_keyword_ci(ident, len, "or")) {
    *out_kind = TK_OR; return 1;
  }
  if (is_keyword_ci(ident, len, "not")) {
    *out_kind = TK_NOT; return 1;
  }
  if (is_keyword(ident, len)) {
    *out_kind = TK_KEYWORD;
    if (out_kw) *out_kw = NULL; // optional
    return 1;
  }
  return 0;
}

static LiminalSpan make_span(size_t start_pos, size_t end_pos, int start_line,
                             int start_col) {
  LiminalSpan sp;
  sp.offset = (int)start_pos;
  sp.length = (int)(end_pos - start_pos);
  sp.line = start_line;
  sp.column = start_col;
  return sp;
}

Lexer *lexer_create(const char *input, size_t length) {
  Lexer *lx = (Lexer *)calloc(1, sizeof(Lexer));
  lx->src = input;
  lx->len = length;
  lx->pos = 0;
  lx->line = 1;
  lx->col = 1;
  return lx;
}

void lexer_destroy(Lexer *lexer) { free(lexer); }

static int peek(const Lexer *lx) {
  if (lx->pos >= lx->len) return EOF;
  return (unsigned char)lx->src[lx->pos];
}

static int peek_next(const Lexer *lx) {
  if (lx->pos + 1 >= lx->len) return EOF;
  return (unsigned char)lx->src[lx->pos + 1];
}

static int advance(Lexer *lx) {
  if (lx->pos >= lx->len) return EOF;
  int c = (unsigned char)lx->src[lx->pos++];
  if (c == '\n') {
    lx->line += 1;
    lx->col = 1;
  } else {
    lx->col += 1;
  }
  return c;
}

static void skip_whitespace_and_comments(Lexer *lx) {
  for (;;) {
    int c = peek(lx);
    if (isspace(c)) {
      advance(lx);
      continue;
    }
    if (c == '/' && peek_next(lx) == '/') {
      // line comment
      while (c != '\n' && c != EOF) c = advance(lx);
      continue;
    }
    if (c == '/' && peek_next(lx) == '*') {
      advance(lx); advance(lx);
      int prev = 0;
      while ((c = peek(lx)) != EOF) {
        if (prev == '*' && c == '/') { advance(lx); break; }
        prev = advance(lx);
      }
      continue;
    }
    break;
  }
}

static Token make_token(const Lexer *lx, TokenKind kind, size_t start_pos, size_t end_pos,
                        int start_line, int start_col) {
  Token t;
  t.kind = kind;
  t.span = make_span(start_pos, end_pos, start_line, start_col);
  t.lexeme = lx->src + start_pos;
  t.lexeme_len = end_pos - start_pos;
  t.keyword = NULL;
  t.error_msg[0] = '\0';
  return t;
}

static Token make_error(const Lexer *lx, size_t start_pos, size_t end_pos,
                        int start_line, int start_col, const char *msg) {
  Token t = make_token(lx, TK_ERROR, start_pos, end_pos, start_line, start_col);
  snprintf(t.error_msg, sizeof(t.error_msg), "%s", msg);
  return t;
}

static Token lex_number_or_duration_or_money(Lexer *lx, size_t start_pos, int start_line,
                                             int start_col) {
  // Assumes current char is digit
  while (isdigit(peek(lx))) advance(lx);
  int is_real = 0;
  if (peek(lx) == '.' && isdigit(peek_next(lx))) {
    is_real = 1;
    advance(lx); // '.'
    while (isdigit(peek(lx))) advance(lx);
  }
  // Duration suffix check
  size_t save_pos = lx->pos;
  int c1 = peek(lx);
  int c2 = peek_next(lx);
  if (!is_real) {
    if (c1 == 'm' && c2 == 's') {
      advance(lx); advance(lx);
      return make_token(lx, TK_DURATION, start_pos, lx->pos, start_line, start_col);
    }
    if (c1 == 's' || c1 == 'm' || c1 == 'h') {
      advance(lx);
      return make_token(lx, TK_DURATION, start_pos, lx->pos, start_line, start_col);
    }
  }
  lx->pos = save_pos;
  // restore line/col? Not necessary, we didn't advance after save if not duration
  if (is_real) {
    return make_token(lx, TK_REAL, start_pos, lx->pos, start_line, start_col);
  } else {
    return make_token(lx, TK_INTEGER, start_pos, lx->pos, start_line, start_col);
  }
}

static Token lex_money(Lexer *lx, size_t start_pos, int start_line, int start_col) {
  // current char is '$'
  advance(lx); // consume $
  if (!isdigit(peek(lx))) {
    return make_error(lx, start_pos, lx->pos, start_line, start_col, "Invalid money literal");
  }
  while (isdigit(peek(lx))) advance(lx);
  if (peek(lx) == '.' && isdigit(peek_next(lx))) {
    advance(lx);
    while (isdigit(peek(lx))) advance(lx);
  }
  return make_token(lx, TK_MONEY, start_pos, lx->pos, start_line, start_col);
}

static int is_escape_seq_char(int c) {
  return c == '\\' || c == '\'' || c == '"' || c == 'n' || c == 'r' || c == 't' || c == 'x';
}

static Token lex_string_like(Lexer *lx, size_t start_pos, int start_line, int start_col,
                             TokenKind kind) {
  // starting quote already consumed by caller? We design to consume here.
  // Here start_pos points to quote
  advance(lx); // consume opening quote
  int escaped = 0;
  size_t content_len = 0;
  while (1) {
    int c = peek(lx);
    if (c == EOF || c == '\n') {
      return make_error(lx, start_pos, lx->pos, start_line, start_col, "Unterminated string");
    }
    if (!escaped && c == '\'') {
      advance(lx); // consume closing quote
      break;
    }
    if (!escaped && c == '\\') {
      escaped = 1;
      advance(lx);
      continue;
    }
    if (escaped) {
      if (!is_escape_seq_char(c)) {
        // still accept but could warn; treat as normal char
      }
      escaped = 0;
    }
    content_len++;
    advance(lx);
  }
  Token t = make_token(lx, kind, start_pos, lx->pos, start_line, start_col);
  // For char detection: we could compute logical length, but approximate by content_len
  if (kind == TK_STRING && content_len == 1) {
    t.kind = TK_CHAR;
  }
  return t;
}

static Token lex_prefixed_string(Lexer *lx, size_t start_pos, int start_line, int start_col,
                                 TokenKind kind) {
  // caller: prefix char consumed? No, we pass start_pos at prefix, so consume prefix char now
  advance(lx); // consume prefix (b or f)
  if (peek(lx) != '\'') {
    return make_error(lx, start_pos, lx->pos, start_line, start_col, "Expected quote after prefix");
  }
  // quote consumed inside lex_string_like
  return lex_string_like(lx, start_pos, start_line, start_col, kind);
}

Token lexer_next(Lexer *lx) {
  skip_whitespace_and_comments(lx);
  size_t start = lx->pos;
  int start_line = lx->line;
  int start_col = lx->col;
  int c = peek(lx);
  if (c == EOF) {
    return make_token(lx, TK_EOF, start, start, start_line, start_col);
  }

  // Money literal
  if (c == '$') {
    return lex_money(lx, start, start_line, start_col);
  }

  // Prefixed literals
  if (c == 'b' || c == 'B') {
    if (peek_next(lx) == '\'') {
      return lex_prefixed_string(lx, start, start_line, start_col, TK_BYTES);
    }
  }
  if (c == 'f' || c == 'F') {
    if (peek_next(lx) == '\'') {
      return lex_prefixed_string(lx, start, start_line, start_col, TK_FSTRING);
    }
  }

  // Identifiers / keywords
  if (is_ident_start(c)) {
    advance(lx);
    while (is_ident_part(peek(lx))) advance(lx);
    size_t len = lx->pos - start;
    TokenKind k;
    const char *kw = NULL;
    if (ident_to_token_kind(lx->src + start, len, &k, &kw)) {
      Token t = make_token(lx, k, start, lx->pos, start_line, start_col);
      t.keyword = kw;
      return t;
    }
    Token t = make_token(lx, TK_IDENTIFIER, start, lx->pos, start_line, start_col);
    return t;
  }

  // Numbers / duration
  if (isdigit(c)) {
    return lex_number_or_duration_or_money(lx, start, start_line, start_col);
  }

  // Strings / chars
  if (c == '\'') {
    return lex_string_like(lx, start, start_line, start_col, TK_STRING);
  }

  // Operators and punctuation
  switch (c) {
  case '?':
    advance(lx);
    return make_token(lx, TK_QMARK, start, lx->pos, start_line, start_col);
  case '!':
    advance(lx);
    return make_token(lx, TK_BANG, start, lx->pos, start_line, start_col);
  case ':':
    if (peek_next(lx) == '=') {
      advance(lx); advance(lx);
      return make_token(lx, TK_ASSIGN, start, lx->pos, start_line, start_col);
    }
    advance(lx);
    return make_token(lx, TK_COLON, start, lx->pos, start_line, start_col);
  case ';':
    advance(lx);
    return make_token(lx, TK_SEMICOLON, start, lx->pos, start_line, start_col);
  case ',':
    advance(lx);
    return make_token(lx, TK_COMMA, start, lx->pos, start_line, start_col);
  case '.':
    if (peek_next(lx) == '.') {
      advance(lx); advance(lx);
      return make_token(lx, TK_DOT_DOT, start, lx->pos, start_line, start_col);
    }
    advance(lx);
    return make_token(lx, TK_DOT, start, lx->pos, start_line, start_col);
  case '(': advance(lx); return make_token(lx, TK_LPAREN, start, lx->pos, start_line, start_col);
  case ')': advance(lx); return make_token(lx, TK_RPAREN, start, lx->pos, start_line, start_col);
  case '[': advance(lx); return make_token(lx, TK_LBRACKET, start, lx->pos, start_line, start_col);
  case ']': advance(lx); return make_token(lx, TK_RBRACKET, start, lx->pos, start_line, start_col);
  case '{': advance(lx); return make_token(lx, TK_LBRACE, start, lx->pos, start_line, start_col);
  case '}': advance(lx); return make_token(lx, TK_RBRACE, start, lx->pos, start_line, start_col);
  case '+': advance(lx); return make_token(lx, TK_PLUS, start, lx->pos, start_line, start_col);
  case '-': advance(lx); return make_token(lx, TK_MINUS, start, lx->pos, start_line, start_col);
  case '*': advance(lx); return make_token(lx, TK_STAR, start, lx->pos, start_line, start_col);
  case '/': advance(lx); return make_token(lx, TK_SLASH, start, lx->pos, start_line, start_col);
  case '=': advance(lx); return make_token(lx, TK_EQ, start, lx->pos, start_line, start_col);
  case '<':
    if (peek_next(lx) == '>') { advance(lx); advance(lx); return make_token(lx, TK_NEQ, start, lx->pos, start_line, start_col); }
    if (peek_next(lx) == '=') { advance(lx); advance(lx); return make_token(lx, TK_LE, start, lx->pos, start_line, start_col); }
    advance(lx); return make_token(lx, TK_LT, start, lx->pos, start_line, start_col);
  case '>':
    if (peek_next(lx) == '=') { advance(lx); advance(lx); return make_token(lx, TK_GE, start, lx->pos, start_line, start_col); }
    advance(lx); return make_token(lx, TK_GT, start, lx->pos, start_line, start_col);
  default:
    break;
  }

  // Unknown character
  advance(lx);
  return make_error(lx, start, lx->pos, start_line, start_col, "Unexpected character");
}

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
  case TK_EOF: return "EOF";
  case TK_IDENTIFIER: return "IDENT";
  case TK_KEYWORD: return "KEYWORD";
  case TK_INTEGER: return "INTEGER";
  case TK_REAL: return "REAL";
  case TK_STRING: return "STRING";
  case TK_BYTES: return "BYTES";
  case TK_CHAR: return "CHAR";
  case TK_FSTRING: return "FSTRING";
  case TK_DURATION: return "DURATION";
  case TK_MONEY: return "MONEY";
  case TK_BOOL: return "BOOL";
  case TK_COLON: return ":";
  case TK_SEMICOLON: return ";";
  case TK_COMMA: return ",";
  case TK_DOT: return ".";
  case TK_DOT_DOT: return "..";
  case TK_LPAREN: return "(";
  case TK_RPAREN: return ")";
  case TK_LBRACKET: return "[";
  case TK_RBRACKET: return "]";
  case TK_LBRACE: return "{";
  case TK_RBRACE: return "}";
  case TK_PLUS: return "+";
  case TK_MINUS: return "-";
  case TK_STAR: return "*";
  case TK_SLASH: return "/";
  case TK_DIV: return "DIV";
  case TK_MOD: return "MOD";
  case TK_ASSIGN: return ":=";
  case TK_EQ: return "=";
  case TK_NEQ: return "<>";
  case TK_LT: return "<";
  case TK_GT: return ">";
  case TK_LE: return "<=";
  case TK_GE: return ">=";
  case TK_AND: return "AND";
  case TK_OR: return "OR";
  case TK_NOT: return "NOT";
  case TK_QMARK: return "?";
  case TK_BANG: return "!";
  case TK_ERROR: return "ERROR";
  }
  return "UNKNOWN";
}
