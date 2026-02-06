#include "test_harness.h"

#include <stdio.h>

static int tests_run = 0;
static int tests_failed = 0;

void test_harness_record_failure(void) { tests_failed++; }
void test_harness_record_failure_msg(const char *msg) {
  tests_failed++;
  if (msg) fprintf(stderr, "%s\n", msg);
}

void run_test(const char *name, test_fn fn) {
  int before = tests_failed;
  fn();
  tests_run++;
  if (tests_failed == before) {
    fprintf(stdout, "[PASS] %s\n", name);
  } else {
    fprintf(stdout, "[FAIL] %s\n", name);
  }
}

int get_tests_failed(void) { return tests_failed; }
int get_tests_run(void) { return tests_run; }
