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

static void test_consult_basic(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "hi", NULL);
  exec_set_global_oracle(o);
  char *out=NULL; run_fixture("consult_basic.lim", &out);
  ASSERT_EQ_STR("Ok(hi)\n", out);
  free(out); exec_set_global_oracle(NULL); oracle_free(o);
}

static void test_consult_retry(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  oracle_mock_queue(o, "hi", NULL);
  exec_set_global_oracle(o);
  char *out=NULL; run_fixture("consult_retry.lim", &out);
  ASSERT_EQ_STR("Ok(hi)\n", out);
  free(out); exec_set_global_oracle(NULL); oracle_free(o);
}

static void test_consult_fallback(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  exec_set_global_oracle(o);
  char *out=NULL; run_fixture("consult_fallback.lim", &out);
  ASSERT_EQ_STR("Ok(fb)\n", out);
  free(out); exec_set_global_oracle(NULL); oracle_free(o);
}

static void test_consult_hint(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "{\"Name\":123,\"Age\":\"x\"}", NULL);
  oracle_mock_queue(o, "{\"Name\":\"Bob\",\"Age\":30}", NULL);
  exec_set_global_oracle(o);
  char *out=NULL; run_fixture("consult_hint.lim", &out);
  ASSERT_EQ_STR("Ok({\"Name\":\"Bob\",\"Age\":30})\n", out);
  free(out); exec_set_global_oracle(NULL); oracle_free(o);
}

int main(void) {
  run_test("consult_basic", test_consult_basic);
  run_test("consult_retry", test_consult_retry);
  run_test("consult_fallback", test_consult_fallback);
  run_test("consult_hint", test_consult_hint);
  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "Consult tests passed (%d)\n", get_tests_run());
  return 0;
}
