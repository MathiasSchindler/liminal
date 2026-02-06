#define _POSIX_C_SOURCE 200809L
#include "liminal/parser.h"
#include "liminal/schema.h"
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

static void test_schema_basic(void) {
  char path_src[256]; snprintf(path_src, sizeof(path_src), "%s/tests/fixtures/schema_basic.lim", SOURCE_DIR);
  char path_json[256]; snprintf(path_json, sizeof(path_json), "%s/tests/fixtures/schema_basic.json", SOURCE_DIR);
  char *src = read_all(path_src);
  ASSERT_TRUE(src != NULL);
  Parser *p = parser_create(src, strlen(src));
  ASTNode *prog = parse_program(p);
  ASSERT_TRUE(prog->as.program.types.len == 1);
  ASTType *stype = prog->as.program.types.items[0]->as.type_decl.type;
  char *json = schema_to_json(stype);
  char *expected = read_all(path_json);
  ASSERT_TRUE(expected != NULL);
  if (strcmp(json, expected) != 0) {
    fprintf(stderr, "Expected:\n%s\nGot:\n%s\n", expected, json);
    ASSERT_EQ_STR(expected, json);
  }
  free(src); free(json); free(expected);
  ast_free(prog); parser_destroy(p);
}

int main(void) {
  run_test("schema_basic", test_schema_basic);
  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "Schema emit tests passed (%d)\n", get_tests_run());
  return 0;
}
