#define _POSIX_C_SOURCE 200809L
#include "liminal/parser.h"
#include "liminal/ir.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char *buf = malloc(len + 1);
  fread(buf, 1, len, f);
  buf[len] = '\0';
  fclose(f);
  return buf;
}

static void assert_ir_matches(const char *fixture_base) {
  char path_src[256]; snprintf(path_src, sizeof(path_src), "%s/tests/fixtures/%s.lim", SOURCE_DIR, fixture_base);
  char path_ir[256]; snprintf(path_ir, sizeof(path_ir), "%s/tests/fixtures/%s.ir", SOURCE_DIR, fixture_base);
  char *src = read_all(path_src);
  ASSERT_TRUE(src != NULL);
  Parser *p = parser_create(src, strlen(src));
  ASTNode *prog = parse_program(p);
  IrProgram *ir = ir_from_ast(prog);
  ASSERT_TRUE(ir != NULL);
  char *errmsg = NULL;
  ASSERT_TRUE(ir_validate(ir, &errmsg));
  if (errmsg) free(errmsg);
  char *printed = ir_program_print(ir);
  char *expected = read_all(path_ir);
  ASSERT_TRUE(expected != NULL);
  if (strcmp(printed, expected) != 0) {
    fprintf(stderr, "Expected IR:\n%s\nGot IR:\n%s\n", expected, printed);
    ASSERT_EQ_STR(expected, printed);
  }
  free(src); free(printed); free(expected);
  ir_program_free(ir);
  ast_free(prog);
  parser_destroy(p);
}

static void test_ir_basic(void) { assert_ir_matches("ir_basic"); }
static void test_ir_if(void) { assert_ir_matches("ir_if"); }
static void test_ir_ask(void) { assert_ir_matches("ir_ask"); }

int main(void) {
  run_test("ir_basic", test_ir_basic);
  run_test("ir_if", test_ir_if);
  run_test("ir_ask", test_ir_ask);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "IR tests passed (%d)\n", get_tests_run());
  return 0;
}
