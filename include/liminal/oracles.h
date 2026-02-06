#ifndef LIMINAL_ORACLES_H
#define LIMINAL_ORACLES_H

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ORACLE_KIND_NONE = 0,
  ORACLE_KIND_MOCK,
  ORACLE_KIND_OLLAMA
} OracleKind;

typedef struct {
  int ok;           // 1 success, 0 failure
  char *error;      // nullable
  char *text;       // nullable (text response)
  float *embedding; // nullable (embedding response)
  size_t embedding_len;
} OracleResult;

typedef struct Oracle {
  OracleKind kind;
  void *impl;
  OracleResult (*call_text)(void *impl, const char *prompt);
  void (*destroy)(void *impl);
} Oracle;

// Core
OracleResult oracle_call_text(Oracle *o, const char *prompt);
void oracle_result_free(OracleResult r);
void oracle_free(Oracle *o);

// Mock provider
Oracle *oracle_create_mock(void);
void oracle_mock_queue(Oracle *o, const char *text_or_null, const char *error_or_null);

// Ollama provider (text only)
Oracle *oracle_create_ollama(const char *endpoint, const char *model);
int oracle_ollama_available(void);

// Recording/replay wrapper: mode = "live"|"record"|"replay"
Oracle *oracle_with_recording(Oracle *inner, const char *mode, const char *path);

// Config loader (env + liminal.ini if present)
Oracle *oracle_from_env(void);

// Internal helper
Oracle *oracle_alloc(OracleKind kind, void *impl,
                    OracleResult (*call)(void*, const char*),
                    void (*destroy)(void*));

// Utilities
char *oracle_canonicalize_prompt(const char *prompt);
void oracle_hash_prompt(const char *canon, char out_hex[65]);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_ORACLES_H
