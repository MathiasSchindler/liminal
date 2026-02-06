#define _POSIX_C_SOURCE 200809L
#include "liminal/parser.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *read_all(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char *buf = malloc(len + 1);
  fread(buf, 1, len, f);
  buf[len] = '\0';
  fclose(f);
  if (out_len) *out_len = (size_t)len;
  return buf;
}

static char *print_ast_to_string(const ASTNode *node) {
  char tmpl[] = "/tmp/astXXXXXX";
  int fd = mkstemp(tmpl);
  if (fd == -1) return NULL;
  FILE *f = fdopen(fd, "w+");
  if (!f) { close(fd); unlink(tmpl); return NULL; }
  ast_print(node, f);
  fflush(f);
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  rewind(f);
  char *out = malloc(n + 1);
  fread(out, 1, n, f);
  out[n] = '\0';
  fclose(f);
  unlink(tmpl);
  return out;
}

static void assert_parse_matches(const char *fixture_base) {
  char path_src[256];
  char path_ast[256];
  char path_err[256];
  snprintf(path_src, sizeof(path_src), "%s/tests/fixtures/%s.lim", SOURCE_DIR, fixture_base);
  snprintf(path_ast, sizeof(path_ast), "%s/tests/fixtures/%s.ast", SOURCE_DIR, fixture_base);
  snprintf(path_err, sizeof(path_err), "%s/tests/fixtures/%s.errors", SOURCE_DIR, fixture_base);

  size_t src_len = 0;
  char *src = read_all(path_src, &src_len);
  if (!src) { test_harness_record_failure_msg("failed to read source"); return; }

  Parser *p = parser_create(src, src_len);
  ASTNode *prog = parse_program(p);
  int has_errors = parser_has_errors(p);

  char *ast_str = print_ast_to_string(prog);
  char *expected_err = NULL;
  char *expected_ast = read_all(path_ast, NULL);
  if (!expected_ast) {
    test_harness_record_failure_msg("missing expected AST");
    goto cleanup;
  }
  if (strcmp(ast_str, expected_ast) != 0) {
    fprintf(stderr, "--- expected AST ---\n%s\n--- actual AST ---\n%s\n", expected_ast, ast_str);
    test_harness_record_failure_msg("AST mismatch");
    goto cleanup;
  }

  expected_err = read_all(path_err, NULL); // may be NULL
  if (expected_err) {
    if (!has_errors) test_harness_record_failure_msg("expected errors but none");
  } else {
    if (has_errors) test_harness_record_failure_msg("unexpected errors");
  }

cleanup:
  free(src);
  free(ast_str);
  if (expected_ast) free(expected_ast);
  if (expected_err) free(expected_err);
  ast_free(prog);
  parser_destroy(p);
}

static void test_parser_basic(void) { assert_parse_matches("parser_basic"); }
static void test_parser_exprs(void) { assert_parse_matches("parser_exprs"); }
static void test_parser_errors(void) { assert_parse_matches("parser_errors"); }
static void test_parser_ask(void) { assert_parse_matches("parser_ask"); }

int main(void) {
  run_test("parser_basic", test_parser_basic);
  run_test("parser_exprs", test_parser_exprs);
  run_test("parser_errors", test_parser_errors);
  run_test("parser_ask", test_parser_ask);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "All parser tests passed (%d)\n", get_tests_run());
  return 0;
}
