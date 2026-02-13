#define _POSIX_C_SOURCE 200809L
#include "liminal/exec.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_exec_hello(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/tests/fixtures/exec_hello.lim", SOURCE_DIR);
  char *outbuf = NULL; size_t outlen = 0;
  FILE *out = open_memstream(&outbuf, &outlen);
  int rc = liminal_run_file_streams(path, NULL, out);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
  ASSERT_EQ_STR("Hello, World!\n", outbuf);
  free(outbuf);
}

static void test_exec_add(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/tests/fixtures/exec_add.lim", SOURCE_DIR);
  char *outbuf = NULL; size_t outlen = 0;
  FILE *out = open_memstream(&outbuf, &outlen);
  const char *in_data = "3\n4\n";
  FILE *in = fmemopen((void*)in_data, strlen(in_data), "r");
  int rc = liminal_run_file_streams(path, in, out);
  fclose(in);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
  ASSERT_EQ_STR("First number: Second number: Sum: 7\n", outbuf);
  free(outbuf);
}

static void test_exec_opus_t17_array_regression(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/examples/opus/t17_array.lim", SOURCE_DIR);
  char *outbuf = NULL; size_t outlen = 0;
  FILE *out = open_memstream(&outbuf, &outlen);
  int rc = liminal_run_file_streams(path, NULL, out);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
  ASSERT_EQ_STR("10\n50\n", outbuf);
  free(outbuf);
}

static void test_exec_opus_c03_traffic_light_regression(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/examples/opus/c03_traffic_light.lim", SOURCE_DIR);
  char *outbuf = NULL; size_t outlen = 0;
  FILE *out = open_memstream(&outbuf, &outlen);
  int rc = liminal_run_file_streams(path, NULL, out);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
  ASSERT_CONTAINS(outbuf, "Step 1: RED");
  ASSERT_CONTAINS(outbuf, "Step 6: YELLOW");
  free(outbuf);
}

static void test_exec_opus_c07_gcd_lcm_regression(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/examples/opus/c07_gcd_lcm.lim", SOURCE_DIR);
  char *outbuf = NULL; size_t outlen = 0;
  FILE *out = open_memstream(&outbuf, &outlen);
  int rc = liminal_run_file_streams(path, NULL, out);
  fflush(out); fclose(out);
  ASSERT_TRUE(rc == 0);
  ASSERT_CONTAINS(outbuf, "GCD(12, 8) = 4");
  ASSERT_CONTAINS(outbuf, "LCM(12, 18) = 36");
  free(outbuf);
}

int main(void) {
  run_test("exec_hello", test_exec_hello);
  run_test("exec_add", test_exec_add);
  run_test("exec_opus_t17_array_regression", test_exec_opus_t17_array_regression);
  run_test("exec_opus_c03_traffic_light_regression", test_exec_opus_c03_traffic_light_regression);
  run_test("exec_opus_c07_gcd_lcm_regression", test_exec_opus_c07_gcd_lcm_regression);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "Exec tests passed (%d)\n", get_tests_run());
  return 0;
}
