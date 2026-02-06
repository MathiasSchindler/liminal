#define _POSIX_C_SOURCE 200809L
#include "liminal/lexer.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); return NULL; }
  fread(buf, 1, (size_t)len, f);
  buf[len] = '\0';
  fclose(f);
  if (out_len) *out_len = (size_t)len;
  return buf;
}

static char *dump_tokens(const char *src, size_t len) {
  Lexer *lx = lexer_create(src, len);
  size_t cap = 1024;
  size_t used = 0;
  char *out = (char *)malloc(cap);
  if (!out) return NULL;
  out[0] = '\0';
  for (;;) {
    Token t = lexer_next(lx);
    char line[256];
    if (t.kind == TK_ERROR) {
      snprintf(line, sizeof(line), "%s '%.*s' @%d:%d (%s)\n", token_kind_name(t.kind),
               (int)t.lexeme_len, t.lexeme, t.span.line, t.span.column, t.error_msg);
    } else {
      snprintf(line, sizeof(line), "%s '%.*s' @%d:%d\n", token_kind_name(t.kind),
               (int)t.lexeme_len, t.lexeme, t.span.line, t.span.column);
    }
    size_t linelen = strlen(line);
    if (used + linelen + 1 > cap) {
      cap *= 2;
      out = (char *)realloc(out, cap);
      if (!out) { lexer_destroy(lx); return NULL; }
    }
    memcpy(out + used, line, linelen);
    used += linelen;
    out[used] = '\0';
    if (t.kind == TK_EOF) break;
  }
  lexer_destroy(lx);
  return out;
}

static void assert_tokens_match_fixture(const char *fixture_basename) {
  char path_src[256];
  char path_tok[256];
  snprintf(path_src, sizeof(path_src), "%s/tests/fixtures/%s.lim", SOURCE_DIR, fixture_basename);
  snprintf(path_tok, sizeof(path_tok), "%s/tests/fixtures/%s.tokens", SOURCE_DIR, fixture_basename);

  size_t src_len = 0;
  char *src = read_all(path_src, &src_len);
  ASSERT_TRUE(src != NULL);
  char *expected = read_all(path_tok, NULL);
  ASSERT_TRUE(expected != NULL);

  char *actual = dump_tokens(src, src_len);
  ASSERT_TRUE(actual != NULL);

  if (strcmp(actual, expected) != 0) {
    fprintf(stderr, "--- Expected ---\n%s\n--- Actual ---\n%s\n", expected, actual);
    ASSERT_EQ_STR(expected, actual);
  }

  free(src);
  free(expected);
  free(actual);
}

static void test_basic_program(void) {
  assert_tokens_match_fixture("lexer_basic");
}

static void test_literals(void) {
  assert_tokens_match_fixture("lexer_literals");
}

static void test_errors(void) {
  assert_tokens_match_fixture("lexer_errors");
}

int main(void) {
  run_test("lexer_basic_program", test_basic_program);
  run_test("lexer_literals", test_literals);
  run_test("lexer_errors", test_errors);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "All lexer tests passed (%d)\n", get_tests_run());
  return 0;
}
