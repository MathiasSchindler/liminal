#include "liminal/lexer.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __AFL_FUZZ_TESTCASE_LEN
#include "afl-fuzz.h"
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  Lexer *lx = lexer_create((const char *)data, size);
  for (;;) {
    Token t = lexer_next(lx);
    if (t.kind == TK_EOF || t.kind == TK_ERROR) break;
  }
  lexer_destroy(lx);
  return 0;
}

#ifndef __LIBFUZZER__
int main(int argc, char **argv) {
  // Simple stdin corpus runner
  fseek(stdin, 0, SEEK_END);
  long len = ftell(stdin);
  rewind(stdin);
  char *buf = malloc(len);
  if (!buf) return 1;
  fread(buf, 1, len, stdin);
  LLVMFuzzerTestOneInput((const uint8_t *)buf, len);
  free(buf);
  return 0;
}
#endif
