#define _POSIX_C_SOURCE 200809L
#include "liminal/oracles.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_mock_success(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, "hi", NULL);
  OracleResult r = oracle_call_text(o, "prompt");
  ASSERT_TRUE(r.ok == 1);
  ASSERT_EQ_STR("hi", r.text);
  oracle_result_free(r);
  oracle_free(o);
}

static void test_mock_failure(void) {
  Oracle *o = oracle_create_mock();
  oracle_mock_queue(o, NULL, "boom");
  OracleResult r = oracle_call_text(o, "prompt");
  ASSERT_TRUE(r.ok == 0);
  ASSERT_TRUE(r.error != NULL);
  oracle_result_free(r);
  oracle_free(o);
}

static void test_record_replay(void) {
  const char *path = "/tmp/oracle_rec_test.jsonl";
  unlink(path);
  Oracle *base = oracle_create_mock();
  oracle_mock_queue(base, "world", NULL);
  Oracle *rec = oracle_with_recording(base, "record", path);
  OracleResult r1 = oracle_call_text(rec, "Hello\nWorld");
  ASSERT_TRUE(r1.ok == 1);
  ASSERT_EQ_STR("world", r1.text);
  oracle_result_free(r1);
  oracle_free(rec); // frees base

  Oracle *rep = oracle_with_recording(NULL, "replay", path);
  OracleResult r2 = oracle_call_text(rep, "Hello  World\r\n");
  ASSERT_TRUE(r2.ok == 1);
  ASSERT_EQ_STR("world", r2.text);
  oracle_result_free(r2);
  oracle_free(rep);
}

static void test_env_loader_mock(void) {
  setenv("LIMINAL_ORACLE_PROVIDER", "mock", 1);
  Oracle *o = oracle_from_env();
  oracle_mock_queue(o, "ok", NULL);
  OracleResult r = oracle_call_text(o, "p");
  ASSERT_TRUE(r.ok == 1);
  ASSERT_EQ_STR("ok", r.text);
  oracle_result_free(r);
  oracle_free(o);
}

static void test_ollama_integration(void) {
  const char *flag = getenv("LIMINAL_OLLAMA_TEST");
  if (!flag || strcmp(flag, "1") != 0) {
    // skip
    return;
  }
  Oracle *o = oracle_create_ollama(getenv("LIMINAL_OLLAMA_ENDPOINT") ? getenv("LIMINAL_OLLAMA_ENDPOINT") : "http://localhost:11434",
                                   getenv("LIMINAL_OLLAMA_MODEL") ? getenv("LIMINAL_OLLAMA_MODEL") : "llama3");
  if (!o) return;
  OracleResult r = oracle_call_text(o, "Say 'pong'.");
  if (r.ok) {
    // should contain pong (case-insensitive)
    int ok = (strstr(r.text, "pong") || strstr(r.text, "Pong")) ? 1 : 0;
    ASSERT_TRUE(ok == 1);
  }
  oracle_result_free(r);
  oracle_free(o);
}

int main(void) {
  run_test("mock_success", test_mock_success);
  run_test("mock_failure", test_mock_failure);
  run_test("record_replay", test_record_replay);
  run_test("env_loader_mock", test_env_loader_mock);
  run_test("ollama_integration", test_ollama_integration);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "Oracle tests passed (%d)\n", get_tests_run());
  return 0;
}
