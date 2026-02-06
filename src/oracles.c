#define _POSIX_C_SOURCE 200809L
#include "liminal/oracles.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "liminal/sha256.h"

static Oracle *oracle_new(void) {
  Oracle *o = (Oracle *)calloc(1, sizeof(Oracle));
  return o;
}

OracleResult oracle_call_text(Oracle *o, const char *prompt) {
  if (!o || !o->call_text) {
    OracleResult r = {0};
    r.ok = 0;
    r.error = strdup("oracle not available");
    return r;
  }
  return o->call_text(o->impl, prompt);
}

void oracle_result_free(OracleResult r) {
  free(r.error);
  free(r.text);
  free(r.embedding);
}

void oracle_free(Oracle *o) {
  if (!o) return;
  if (o->destroy) o->destroy(o->impl);
  free(o);
}

// Utilities
char *oracle_canonicalize_prompt(const char *prompt) {
  if (!prompt) return strdup("");
  size_t len = strlen(prompt);
  char *buf = (char *)malloc(len + 1);
  size_t j = 0;
  int prev_space = 0;
  for (size_t i = 0; i < len; ++i) {
    char c = prompt[i];
    if (c == '\r') continue; // normalize CRLF -> LF
    if (isspace((unsigned char)c)) {
      if (!prev_space) buf[j++] = ' ';
      prev_space = 1;
    } else {
      buf[j++] = c;
      prev_space = 0;
    }
  }
  // trim trailing space
  while (j > 0 && buf[j-1] == ' ') j--;
  // trim leading space
  size_t start = 0;
  while (start < j && buf[start] == ' ') start++;
  size_t out_len = j - start;
  char *out = (char *)malloc(out_len + 1);
  memcpy(out, buf + start, out_len);
  out[out_len] = '\0';
  free(buf);
  return out;
}

void oracle_hash_prompt(const char *canon, char out_hex[65]) {
  uint8_t digest[32];
  sha256((const uint8_t *)canon, strlen(canon), digest);
  sha256_hex(digest, out_hex);
}

// Minimal config loader (env + liminal.ini)
static int file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static void trim(char *s) {
  size_t len = strlen(s);
  size_t start = 0;
  while (start < len && isspace((unsigned char)s[start])) start++;
  size_t end = len;
  while (end > start && isspace((unsigned char)s[end-1])) end--;
  if (start > 0) memmove(s, s + start, end - start);
  s[end - start] = '\0';
}

typedef struct {
  char provider[32];
  char endpoint[256];
  char model[128];
  char mode[16];
  char recording[256];
} OracleEnvConfig;

static void load_ini(const char *path, OracleEnvConfig *cfg) {
  FILE *f = fopen(path, "r");
  if (!f) return;
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == ';' || line[0] == '\n') continue;
    char *eq = strchr(line, '=');
    if (!eq) continue;
    *eq = '\0';
    char *key = line;
    char *val = eq + 1;
    trim(key); trim(val);
    if (strcmp(key, "provider") == 0) strncpy(cfg->provider, val, sizeof(cfg->provider)-1);
    else if (strcmp(key, "endpoint") == 0) strncpy(cfg->endpoint, val, sizeof(cfg->endpoint)-1);
    else if (strcmp(key, "model") == 0) strncpy(cfg->model, val, sizeof(cfg->model)-1);
    else if (strcmp(key, "mode") == 0) strncpy(cfg->mode, val, sizeof(cfg->mode)-1);
    else if (strcmp(key, "recording") == 0) strncpy(cfg->recording, val, sizeof(cfg->recording)-1);
  }
  fclose(f);
}

// Forward decls for providers
Oracle *oracle_create_mock(void);
Oracle *oracle_create_ollama(const char *endpoint, const char *model);
Oracle *oracle_with_recording(Oracle *inner, const char *mode, const char *path);
int oracle_ollama_available(void);

Oracle *oracle_from_env(void) {
  OracleEnvConfig cfg = {0};
  strncpy(cfg.provider, getenv("LIMINAL_ORACLE_PROVIDER") ? getenv("LIMINAL_ORACLE_PROVIDER") : "mock", sizeof(cfg.provider)-1);
  strncpy(cfg.endpoint, getenv("LIMINAL_OLLAMA_ENDPOINT") ? getenv("LIMINAL_OLLAMA_ENDPOINT") : "http://localhost:11434", sizeof(cfg.endpoint)-1);
  strncpy(cfg.model, getenv("LIMINAL_OLLAMA_MODEL") ? getenv("LIMINAL_OLLAMA_MODEL") : "gemma3:12b", sizeof(cfg.model)-1);
  strncpy(cfg.mode, getenv("LIMINAL_ORACLE_MODE") ? getenv("LIMINAL_ORACLE_MODE") : "live", sizeof(cfg.mode)-1);
  strncpy(cfg.recording, getenv("LIMINAL_ORACLE_RECORDING") ? getenv("LIMINAL_ORACLE_RECORDING") : "oracle_recordings.jsonl", sizeof(cfg.recording)-1);

  if (file_exists("liminal.ini")) {
    load_ini("liminal.ini", &cfg);
  }

  Oracle *base = NULL;
  if (strcasecmp(cfg.provider, "mock") == 0) {
    base = oracle_create_mock();
  } else if (strcasecmp(cfg.provider, "ollama") == 0) {
    base = oracle_create_ollama(cfg.endpoint, cfg.model);
  } else {
    base = oracle_create_mock();
  }
  if (!base) return NULL;
  if (strcasecmp(cfg.mode, "live") == 0) return base;
  return oracle_with_recording(base, cfg.mode, cfg.recording);
}

// Factories used by providers
Oracle *oracle_alloc(OracleKind kind, void *impl,
                    OracleResult (*call)(void*, const char*),
                    void (*destroy)(void*)) {
  Oracle *o = oracle_new();
  o->kind = kind;
  o->impl = impl;
  o->call_text = call;
  o->destroy = destroy;
  return o;
}

