#define _POSIX_C_SOURCE 200809L
#include "liminal/runtime.h"
#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>

static void test_string_refcount(void) {
  runtime_reset_counters();
  LString *s = lstring_from_cstr("hello");
  lobject_retain((LObject *)s);
  lobject_release((LObject *)s);
  lobject_release((LObject *)s);
  ASSERT_TRUE(runtime_alloc_count() == runtime_free_count());
}

static void test_array_growth(void) {
  runtime_reset_counters();
  LArray *a = larray_new(1);
  for (int i = 0; i < 100; ++i) {
    char buf[16]; snprintf(buf, sizeof(buf), "%d", i);
    LString *s = lstring_from_cstr(buf);
    larray_push(a, lvalue_string(s));
    lobject_release((LObject *)s);
  }
  ASSERT_TRUE(a->len == 100);
  ASSERT_TRUE(a->cap >= 100);
  // check a few values
  LValue v = larray_get(a, 42);
  ASSERT_TRUE(v.kind == LVAL_STRING);
  ASSERT_TRUE(v.as.str && v.as.str->len > 0);
  lobject_release((LObject *)a);
  ASSERT_TRUE(runtime_alloc_count() == runtime_free_count());
}

static void test_array_set_release(void) {
  runtime_reset_counters();
  LArray *a = larray_new(2);
  LString *s1 = lstring_from_cstr("a");
  LString *s2 = lstring_from_cstr("b");
  larray_push(a, lvalue_string(s1));
  larray_set(a, 0, lvalue_string(s2));
  lobject_release((LObject *)s1);
  lobject_release((LObject *)s2);
  lobject_release((LObject *)a);
  ASSERT_TRUE(runtime_alloc_count() == runtime_free_count());
}

int main(void) {
  run_test("string_refcount", test_string_refcount);
  run_test("array_growth", test_array_growth);
  run_test("array_set_release", test_array_set_release);

  if (get_tests_failed() > 0) {
    fprintf(stderr, "%d/%d tests failed\n", get_tests_failed(), get_tests_run());
    return 1;
  }
  fprintf(stdout, "All runtime tests passed (%d)\n", get_tests_run());
  return 0;
}
