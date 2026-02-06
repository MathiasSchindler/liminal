#define _POSIX_C_SOURCE 200809L
#include "liminal/oracles.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

// Simple availability check: curl must exist
static int curl_exists(void) {
  return access("/usr/bin/curl", X_OK) == 0 || access("/bin/curl", X_OK) == 0 || access("curl", X_OK) == 0;
}

int oracle_ollama_available(void) {
  return curl_exists();
}

typedef struct {
  char *endpoint;
  char *model;
} Ollama;

static char *json_escape_str(const char *s) {
  if (!s) return strdup("");
  size_t len = strlen(s);
  size_t cap = len * 2 + 1;
  char *out = (char *)malloc(cap);
  size_t j = 0;
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)s[i];
    switch (c) {
    case '\\': if (j+2>=cap) {cap*=2; out=realloc(out,cap);} out[j++]='\\'; out[j++]='\\'; break;
    case '"': if (j+2>=cap){cap*=2; out=realloc(out,cap);} out[j++]='\\'; out[j++]='"'; break;
    case '\n': if (j+2>=cap){cap*=2; out=realloc(out,cap);} out[j++]='\\'; out[j++]='n'; break;
    case '\r': if (j+2>=cap){cap*=2; out=realloc(out,cap);} out[j++]='\\'; out[j++]='r'; break;
    case '\t': if (j+2>=cap){cap*=2; out=realloc(out,cap);} out[j++]='\\'; out[j++]='t'; break;
    default:
      if (c < 0x20) {
        if (j+6>=cap){cap*=2; out=realloc(out,cap);} sprintf(out+j, "\\u%04x", c); j+=6;
      } else {
        if (j+1>=cap){cap*=2; out=realloc(out,cap);} out[j++] = c;
      }
    }
  }
  out[j] = '\0';
  return out;
}

static char *run_curl_and_capture(const char *cmd) {
  FILE *p = popen(cmd, "r");
  if (!p) return NULL;
  char *buf = NULL; size_t cap = 0; size_t len = 0;
  char chunk[512];
  while (!feof(p)) {
    size_t n = fread(chunk, 1, sizeof(chunk), p);
    if (n > 0) {
      if (len + n + 1 > cap) { cap = cap ? cap * 2 : 1024; while (cap < len + n + 1) cap *= 2; buf = (char *)realloc(buf, cap); }
      memcpy(buf + len, chunk, n); len += n; buf[len] = '\0';
    }
  }
  pclose(p);
  return buf;
}

static char *parse_ollama_response(const char *json) {
  // Naive parse: find "response":"..."
  const char *p = strstr(json, "\"response\":");
  if (!p) return NULL;
  p += strlen("\"response\":");
  while (*p && (*p==' ' || *p=='\t')) p++;
  if (*p != '"') return NULL;
  p++;
  const char *start = p;
  int escapes = 0;
  while (*p) {
    if (*p == '\\') { escapes = 1; p += 2; continue; }
    if (*p == '"') break;
    p++;
  }
  if (*p != '"') return NULL;
  size_t len = (size_t)(p - start);
  char *val = (char *)malloc(len + 1);
  memcpy(val, start, len); val[len] = '\0';
  if (!escapes) return val;
  // unescape
  char *w = val; char *r = val;
  while (*r) {
    if (*r == '\\') {
      r++;
      switch (*r) {
      case 'n': *w++='\n'; r++; break;
      case 'r': *w++='\r'; r++; break;
      case 't': *w++='\t'; r++; break;
      case '\\': *w++='\\'; r++; break;
      case '"': *w++='"'; r++; break;
      default: *w++='\\'; break;
      }
    } else {
      *w++ = *r++;
    }
  }
  *w = '\0';
  return val;
}

static OracleResult ollama_call(void *impl, const char *prompt) {
  Ollama *o = (Ollama *)impl;
  OracleResult res = {0};
  if (!curl_exists()) { res.ok=0; res.error=strdup("curl not found"); return res; }
  char *esc_prompt = json_escape_str(prompt);
  char *esc_model = json_escape_str(o->model);
  // Write JSON to tmp file
  char tmpl[] = "/tmp/ollama_req_XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd < 0) { free(esc_prompt); free(esc_model); res.ok=0; res.error=strdup("mkstemp failed"); return res; }
  FILE *f = fdopen(fd, "w");
  if (!f) { close(fd); unlink(tmpl); free(esc_prompt); free(esc_model); res.ok=0; res.error=strdup("fdopen failed"); return res; }
  fprintf(f, "{\"model\":\"%s\",\"prompt\":\"%s\",\"stream\":false}\n", esc_model, esc_prompt);
  fclose(f);
  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "curl -s -X POST %s/api/generate -H 'Content-Type: application/json' --data @%s", o->endpoint, tmpl);
  char *out = run_curl_and_capture(cmd);
  unlink(tmpl);
  free(esc_prompt); free(esc_model);
  if (!out) { res.ok=0; res.error=strdup("curl failed"); return res; }
  char *resp = parse_ollama_response(out);
  if (!resp) { res.ok=0; res.error=strdup("ollama parse failed"); free(out); return res; }
  res.ok = 1;
  res.text = resp;
  free(out);
  return res;
}

static void ollama_destroy(void *impl) {
  Ollama *o = (Ollama *)impl;
  free(o->endpoint);
  free(o->model);
  free(o);
}

Oracle *oracle_create_ollama(const char *endpoint, const char *model) {
  if (!endpoint || !model) return NULL;
  Ollama *o = (Ollama *)calloc(1, sizeof(Ollama));
  o->endpoint = strdup(endpoint);
  o->model = strdup(model);
  return oracle_alloc(ORACLE_KIND_OLLAMA, o, ollama_call, ollama_destroy);
}
