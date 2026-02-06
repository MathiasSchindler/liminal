#define _POSIX_C_SOURCE 200809L
#include "liminal/exec.h"
#include "liminal/oracles.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_fixture(const char *fixture, char **outbuf) {
  char path[256]; snprintf(path, sizeof(path), "%s/tests/fixtures/%s", SOURCE_DIR, fixture);
  size_t outlen = 0;
  FILE *out = open_memstream(outbuf, &outlen);
  int rc = liminal_run_file_streams(path, NULL, out);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
}

static void test_ask_ok(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "hi", NULL);
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_ok.lim", &out);
  ASSERT_EQ_STR("Ok(hi)\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_else(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_else.lim", &out);
  ASSERT_EQ_STR("Ok(fallback)\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_unwrapor(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_unwrap.lim", &out);
  ASSERT_EQ_STR("fb\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_err(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_err.lim", &out);
  ASSERT_EQ_STR("Err(boom)\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_chain(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  oracle_mock_queue(o, "hi", NULL);
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_chain.lim", &out);
  ASSERT_EQ_STR("Ok(hi)\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_into_valid(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "{\"Name\":\"Bob\",\"Age\":30}", NULL);
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_into.lim", &out);
  ASSERT_EQ_STR("Ok({\"Name\":\"Bob\",\"Age\":30})\n", out);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

static void test_ask_into_invalid(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "{\"Name\":123,\"Age\":\"x\"}", NULL);
  exec_set_global_oracle(o);
  char *out = NULL;
  run_fixture("ask_into_invalid.lim", &out);
  ASSERT_TRUE(strstr(out, "Err(extraction failed") != NULL || strstr(out, "Err(") != NULL);
  free(out);
  exec_set_global_oracle(NULL);
  oracle_free(o);
}

int main(void) {
  run_test("ask_ok", test_ask_ok);
  run_test("ask_else", test_ask_else);
  run_test("ask_unwrapor", test_ask_unwrapor);
  run_test("ask_err", test_ask_err);
  run_test("ask_chain", test_ask_chain);
  run_test("ask_into_valid", test_ask_into_valid);
  run_test("ask_into_invalid", test_ask_into_invalid);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "Ask tests passed (%d)\n", get_tests_run());
  return 0;
}
