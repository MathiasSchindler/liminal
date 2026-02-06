#define _POSIX_C_SOURCE 200809L
#include "liminal/oracles.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  char **texts;
  char **errors;
  size_t len;
  size_t cap;
  size_t idx;
} OracleMock;

static OracleResult mock_call(void *impl, const char *prompt) {
  (void)prompt;
  OracleMock *m = (OracleMock *)impl;
  OracleResult r = {0};
  if (m->idx >= m->len) {
    r.ok = 0;
    r.error = strdup("mock: no queued response");
    return r;
  }
  char *t = m->texts[m->idx];
  char *e = m->errors[m->idx];
  m->idx++;
  if (e) {
    r.ok = 0;
    r.error = strdup(e);
    return r;
  }
  r.ok = 1;
  r.text = t ? strdup(t) : strdup("");
  return r;
}

static void mock_destroy(void *impl) {
  OracleMock *m = (OracleMock *)impl;
  for (size_t i = 0; i < m->len; ++i) {
    free(m->texts[i]);
    free(m->errors[i]);
  }
  free(m->texts);
  free(m->errors);
  free(m);
}

Oracle *oracle_create_mock(void) {
  OracleMock *m = (OracleMock *)calloc(1, sizeof(OracleMock));
  return oracle_alloc(ORACLE_KIND_MOCK, m, mock_call, mock_destroy);
}

void oracle_mock_queue(Oracle *o, const char *text_or_null, const char *error_or_null) {
  if (!o || o->kind != ORACLE_KIND_MOCK) return;
  OracleMock *m = (OracleMock *)o->impl;
  if (m->len == m->cap) {
    m->cap = m->cap ? m->cap * 2 : 4;
    m->texts = (char **)realloc(m->texts, m->cap * sizeof(char *));
    m->errors = (char **)realloc(m->errors, m->cap * sizeof(char *));
  }
  m->texts[m->len] = text_or_null ? strdup(text_or_null) : NULL;
  m->errors[m->len] = error_or_null ? strdup(error_or_null) : NULL;
  m->len++;
}
