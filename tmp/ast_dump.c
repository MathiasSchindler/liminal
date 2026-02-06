#include "liminal/parser.h"
#include "liminal/ast.h"
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path, size_t *len_out){ FILE *f=fopen(path, "rb"); if(!f) return NULL; fseek(f,0,SEEK_END); long len=ftell(f); rewind(f); char *buf=malloc(len+1); fread(buf,1,len,f); buf[len]='\0'; fclose(f); if(len_out) *len_out=len; return buf; }

static void indent(int n){ while(n--) fputs("  ", stdout); }
static void print_expr(const ASTExpr *e, int level);
static int count_while(const ASTStmt *s){ if(!s) return 0; switch(s->kind){ case STMT_WHILE: return 1 + count_while(s->as.while_stmt.body); case STMT_BLOCK: { int c=0; for(size_t i=0;i<s->as.block.stmts.len;++i) c+=count_while(s->as.block.stmts.items[i]); return c;} case STMT_IF: return count_while(s->as.if_stmt.then_branch)+count_while(s->as.if_stmt.else_branch); case STMT_FOR: return count_while(s->as.for_stmt.body); default: return 0;} }
static void print_stmt(const ASTStmt *s, int level){ if(!s){ indent(level); puts("<null stmt>"); return;} indent(level); switch(s->kind){ case STMT_ASSIGN: puts("Assign"); indent(level+1); print_expr(s->as.assign.target, level+1); indent(level+1); print_expr(s->as.assign.value, level+1); break; case STMT_IF: puts("If"); indent(level+1); print_expr(s->as.if_stmt.cond, level+1); print_stmt(s->as.if_stmt.then_branch, level+1); print_stmt(s->as.if_stmt.else_branch, level+1); break; case STMT_WHILE: puts("While"); indent(level+1); print_expr(s->as.while_stmt.cond, level+1); print_stmt(s->as.while_stmt.body, level+1); break; case STMT_FOR: puts("For"); break; case STMT_BLOCK: puts("Block"); for(size_t i=0;i<s->as.block.stmts.len;++i) print_stmt(s->as.block.stmts.items[i], level+1); break; case STMT_EXPR: puts("ExprStmt"); print_expr(s->as.expr_stmt.expr, level+1); break; default: puts("<stmt>"); break; }}
static void print_expr(const ASTExpr *e, int level){ if(!e){ indent(level); puts("<null expr>"); return;} indent(level); switch(e->kind){ case EXPR_IDENT: printf("Ident(%.*s)\n", (int)e->as.ident.name.len, e->as.ident.name.data); break; case EXPR_LITERAL: printf("Literal\n"); break; case EXPR_BINARY: printf("Binary(%d)\n", e->as.binary.op); print_expr(e->as.binary.lhs, level+1); print_expr(e->as.binary.rhs, level+1); break; case EXPR_CALL: printf("Call\n"); print_expr(e->as.call.callee, level+1); for(size_t i=0;i<e->as.call.args.len;++i) print_expr(e->as.call.args.items[i], level+1); break; case EXPR_CONCAT: printf("Concat\n"); print_expr(e->as.concat.lhs, level+1); print_expr(e->as.concat.rhs, level+1); break; default: printf("<expr %d>\n", e->kind); break; }}

int main(int argc, char **argv){ if(argc<2){ fprintf(stderr,"usage: %s <file>\n", argv[0]); return 1;} size_t len=0; char *src=read_file(argv[1], &len); if(!src){ perror("read"); return 1;} Parser *p=parser_create(src,len); ASTNode *ast=parse_program(p);
  if(ast && ast->kind==AST_PROGRAM){ printf("Program %s\n", ast->as.program.name.data); printf("Functions (%zu)\n", ast->as.program.functions.len); for(size_t i=0;i<ast->as.program.functions.len;++i){ ASTNode *fn=ast->as.program.functions.items[i]; printf("Function %s\n", fn->as.func_decl.name.data); if(fn->as.func_decl.locals){ printf(" Locals: %zu\n", fn->as.func_decl.locals->vars.len);}
      printf(" WhileCount: %d\n", count_while(fn->as.func_decl.body));
      print_stmt(fn->as.func_decl.body, 1); }
    puts("Body:"); print_stmt(ast->as.program.body, 1);
  }
  ast_free(ast); parser_destroy(p); free(src); return 0; }
