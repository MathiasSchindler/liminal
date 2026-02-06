#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

typedef void (*test_fn)(void);

void run_test(const char *name, test_fn fn);
int get_tests_failed(void);
int get_tests_run(void);
void test_harness_record_failure(void);
void test_harness_record_failure_msg(const char *msg);

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                        \
    if (!(cond)) {                                                            \
      test_harness_record_failure();                                          \
      fprintf(stderr, "Assertion failed: %s:%d: %s\n", __FILE__, __LINE__,   \
              #cond);                                                         \
      return;                                                                 \
    }                                                                         \
  } while (0)

#define ASSERT_EQ_STR(expected, actual)                                        \
  do {                                                                        \
    if (strcmp((expected), (actual)) != 0) {                                  \
      test_harness_record_failure();                                          \
      fprintf(stderr,                                                         \
              "Assertion failed: %s:%d: expected '%s' but got '%s'\n",       \
              __FILE__, __LINE__, (expected), (actual));                      \
      return;                                                                 \
    }                                                                         \
  } while (0)

#define ASSERT_CONTAINS(haystack, needle)                                      \
  do {                                                                        \
    if (strstr((haystack), (needle)) == NULL) {                               \
      test_harness_record_failure();                                          \
      fprintf(stderr,                                                         \
              "Assertion failed: %s:%d: expected to find '%s' in '%s'\n",    \
              __FILE__, __LINE__, (needle), (haystack));                      \
      return;                                                                 \
    }                                                                         \
  } while (0)

#endif // TEST_HARNESS_H
