#include "liminal/lexer.h"
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path, size_t *len_out){ FILE *f=fopen(path, "rb"); if(!f) return NULL; fseek(f,0,SEEK_END); long len=ftell(f); rewind(f); char *buf=malloc(len+1); fread(buf,1,len,f); buf[len]='\0'; fclose(f); if(len_out) *len_out=len; return buf; }

int main(int argc, char **argv){ if(argc<2){ fprintf(stderr,"usage: %s <file>\n", argv[0]); return 1;} size_t len=0; char *src=read_file(argv[1], &len); if(!src){ perror("read"); return 1;} Lexer *lx=lexer_create(src,len); Token tk; int i=0; while((tk=lexer_next(lx)).kind!=TK_EOF){ printf("%3d %-12s '%.*s'\n", ++i, token_kind_name(tk.kind), (int)tk.lexeme_len, tk.lexeme); if(tk.kind==TK_ERROR) break; } lexer_destroy(lx); free(src); return 0; }
