#define _POSIX_C_SOURCE 200809L
#include "liminal/oracles.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

typedef struct {
  Oracle *inner;
  char *mode; // live|record|replay
  char *path;
} OracleRecord;

static void json_escape(FILE *f, const char *s) {
  for (const char *p = s; *p; ++p) {
    unsigned char c = (unsigned char)*p;
    switch (c) {
    case '\\': fputs("\\\\", f); break;
    case '"': fputs("\\\"", f); break;
    case '\n': fputs("\\n", f); break;
    case '\r': fputs("\\r", f); break;
    case '\t': fputs("\\t", f); break;
    default:
      if (c < 0x20) fprintf(f, "\\u%04x", c);
      else fputc(c, f);
    }
  }
}

static OracleResult record_call(void *impl, const char *prompt) {
  OracleRecord *r = (OracleRecord *)impl;
  char *canon = oracle_canonicalize_prompt(prompt);
  char hash[65]; oracle_hash_prompt(canon, hash);

  if (strcasecmp(r->mode, "replay") == 0) {
    FILE *f = fopen(r->path, "r");
    if (!f) {
      OracleResult res = {0}; res.ok = 0; res.error = strdup("replay file not found"); free(canon); return res;
    }
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
      char *h = strstr(line, "\"hash\":\"");
      if (!h) continue;
      h += 8; // move to after "hash":"
      char *hend = strchr(h, '"');
      if (!hend) continue;
      if ((size_t)(hend - h) != 64) continue;
      if (strncmp(h, hash, 64) != 0) continue;
      char *resp = strstr(line, "\"response\":\"");
      if (!resp) { fclose(f); free(canon); OracleResult res={0}; res.ok=0; res.error=strdup("replay response missing"); return res; }
      resp += 12; // after "response":"
      char *rend = strchr(resp, '"');
      if (!rend) { fclose(f); free(canon); OracleResult res={0}; res.ok=0; res.error=strdup("replay malformed"); return res; }
      size_t len = (size_t)(rend - resp);
      char *val = (char *)malloc(len + 1);
      strncpy(val, resp, len); val[len] = '\0';
      // naive unescape of common sequences
      // (we only escape \\ and \", \n, \r, \t)
      {
        char *p = val, *w = val;
        while (*p) {
          if (*p == '\\') {
            p++;
            if (*p == 'n') { *w++ = '\n'; p++; }
            else if (*p == 'r') { *w++ = '\r'; p++; }
            else if (*p == 't') { *w++ = '\t'; p++; }
            else if (*p == '\\') { *w++ = '\\'; p++; }
            else if (*p == '"') { *w++ = '"'; p++; }
            else { *w++ = '\\'; }
          } else {
            *w++ = *p++;
          }
        }
        *w = '\0';
      }
      fclose(f); free(canon);
      OracleResult res = {0}; res.ok = 1; res.text = val; return res;
    }
    fclose(f);
    free(canon);
    OracleResult res = {0}; res.ok = 0; res.error = strdup("replay: prompt not found"); return res;
  }

  // live/record
  OracleResult inner = oracle_call_text(r->inner, prompt);
  if (strcasecmp(r->mode, "record") == 0) {
    FILE *f = fopen(r->path, "a");
    if (f) {
      fprintf(f, "{\"hash\":\"%s\",\"prompt\":\"", hash);
      json_escape(f, canon);
      fprintf(f, "\",\"response\":\"");
      if (inner.ok && inner.text) json_escape(f, inner.text);
      else if (!inner.ok && inner.error) json_escape(f, inner.error);
      fprintf(f, "\",\"ok\":%s}\n", inner.ok ? "true" : "false");
      fclose(f);
    }
  }
  free(canon);
  return inner;
}

static void record_destroy(void *impl) {
  OracleRecord *r = (OracleRecord *)impl;
  oracle_free(r->inner);
  free(r->mode);
  free(r->path);
  free(r);
}

Oracle *oracle_with_recording(Oracle *inner, const char *mode, const char *path) {
  OracleRecord *r = (OracleRecord *)calloc(1, sizeof(OracleRecord));
  r->inner = inner;
  r->mode = strdup(mode ? mode : "live");
  r->path = strdup(path ? path : "oracle_recordings.jsonl");
  return oracle_alloc(inner ? inner->kind : ORACLE_KIND_NONE, r, record_call, record_destroy);
}
