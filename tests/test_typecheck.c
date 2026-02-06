#define _POSIX_C_SOURCE 200809L
#include "liminal/parser.h"
#include "liminal/typecheck.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TypeCheckResult check_src(const char *src) {
  Parser *p = parser_create(src, strlen(src));
  ASTNode *prog = parse_program(p);
  TypeCheckResult res = typecheck_program(prog);
  ast_free(prog);
  parser_destroy(p);
  return res;
}

static void test_typecheck_ok(void) {
  const char *src =
      "program Ok;\n"
      "var\n"
      "  X: Integer;\n"
      "  Y: Real;\n"
      "begin\n"
      "  X := 1 + 2;\n"
      "  Y := X + 3.0;\n"
      "end.\n";
  TypeCheckResult res = check_src(src);
  if (!res.ok) {
    fprintf(stderr, "Unexpected errors (%zu)\n", res.errors.len);
    ASSERT_TRUE(res.ok);
  }
  typecheck_result_free(&res);
}

static void test_type_mismatch(void) {
  const char *src =
      "program Bad;\n"
      "var\n"
      "  X: Integer;\n"
      "begin\n"
      "  X := 'hello';\n"
      "end.\n";
  TypeCheckResult res = check_src(src);
  ASSERT_TRUE(!res.ok);
  ASSERT_TRUE(res.errors.len > 0);
  typecheck_result_free(&res);
}

static void test_undeclared(void) {
  const char *src =
      "program Bad;\n"
      "begin\n"
      "  X := 1;\n"
      "end.\n";
  TypeCheckResult res = check_src(src);
  ASSERT_TRUE(!res.ok);
  ASSERT_TRUE(res.errors.len > 0);
  typecheck_result_free(&res);
}

static void test_ask_type_ok(void) {
  const char *src =
      "program P;\n"
      "var S: String;\n"
      "begin\n"
      "  S := ask Oracle <- 'hi';\n"
      "end.\n";
  TypeCheckResult res = check_src(src);
  ASSERT_TRUE(res.ok);
  typecheck_result_free(&res);
}

static void test_ask_type_mismatch(void) {
  const char *src =
      "program P;\n"
      "var I: Integer;\n"
      "begin\n"
      "  I := ask Oracle <- 'hi';\n"
      "end.\n";
  TypeCheckResult res = check_src(src);
  ASSERT_TRUE(!res.ok);
  typecheck_result_free(&res);
}

int main(void) {
  run_test("typecheck_ok", test_typecheck_ok);
  run_test("type_mismatch", test_type_mismatch);
  run_test("undeclared", test_undeclared);
  run_test("ask_type_ok", test_ask_type_ok);
  run_test("ask_type_mismatch", test_ask_type_mismatch);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "All typecheck tests passed (%d)\n", get_tests_run());
  return 0;
}
