#define _POSIX_C_SOURCE 200809L

#include "liminal/cli.h"
#include "test_harness.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *capture_stdout(int (*fn)(int, char **), int argc, char **argv) {
  char tmpl[] = "/tmp/liminal_testXXXXXX";
  int fd = mkstemp(tmpl);
  if (fd == -1) {
    return NULL;
  }
  FILE *tmp = fdopen(fd, "w+");
  if (!tmp) {
    close(fd);
    return NULL;
  }

  int stdout_fd = dup(STDOUT_FILENO);
  fflush(stdout);
  dup2(fd, STDOUT_FILENO);

  fn(argc, argv);

  fflush(stdout);
  dup2(stdout_fd, STDOUT_FILENO);
  close(stdout_fd);

  fseek(tmp, 0, SEEK_END);
  long len = ftell(tmp);
  rewind(tmp);

  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) {
    fclose(tmp);
    unlink(tmpl);
    return NULL;
  }
  fread(buf, 1, (size_t)len, tmp);
  buf[len] = '\0';

  fclose(tmp);
  unlink(tmpl);
  return buf;
}

static void test_help_option_prints_usage(void) {
  char *argv[] = {(char *)"liminal", (char *)"--help", NULL};
  char *out = capture_stdout(liminal_main, 2, argv);
  ASSERT_TRUE(out != NULL);
  ASSERT_CONTAINS(out, "Usage:");
  ASSERT_CONTAINS(out, "liminal - Liminal");
  free(out);
}

static void test_version_option_prints_version(void) {
  char *argv[] = {(char *)"liminal", (char *)"--version", NULL};
  char *out = capture_stdout(liminal_main, 2, argv);
  ASSERT_TRUE(out != NULL);
  ASSERT_EQ_STR("liminal " LIMINAL_VERSION "\n", out);
  free(out);
}

static void test_default_shows_help(void) {
  char *argv[] = {(char *)"liminal", NULL};
  char *out = capture_stdout(liminal_main, 1, argv);
  ASSERT_TRUE(out != NULL);
  ASSERT_CONTAINS(out, "Usage:");
  free(out);
}

static void test_cli_ask_else(void) {
  char path[256]; snprintf(path, sizeof(path), "%s/tests/fixtures/ask_else.lim", SOURCE_DIR);
  char *argv[] = {(char *)"liminal", (char *)"run", path, NULL};
  setenv("LIMINAL_ORACLE_PROVIDER", "mock", 1);
  char *out = capture_stdout(liminal_main, 3, argv);
  ASSERT_TRUE(out != NULL);
  ASSERT_EQ_STR("Ok(fallback)\n", out);
  free(out);
}

int main(void) {
  run_test("help_option_prints_usage", test_help_option_prints_usage);
  run_test("version_option_prints_version", test_version_option_prints_version);
  run_test("default_shows_help", test_default_shows_help);
  run_test("cli_ask_else", test_cli_ask_else);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "All tests passed (%d)\n", get_tests_run());
  return 0;
}
