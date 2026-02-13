#define _POSIX_C_SOURCE 200809L
#include "liminal/exec.h"
#include "liminal/parser.h"
#include "liminal/typecheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int debug_exec(void) {
  const char *dbg = getenv("LIMINAL_DEBUG_EXEC");
  return dbg && *dbg;
}

typedef enum { VINT, VREAL, VSTRING, VRESULT, VBOOL, VOPTIONAL } ValKind;
typedef struct {
  int ok;
  char *text;
  char *error;
} VRes;
typedef struct Value Value;
static size_t _allocs=0, _frees=0;
typedef struct Value {
  ValKind kind;
  int i;
  double f;
  char *s;
  int owns;
  char *ref; // optional reference name
  int ref_interned;
  VRes res;
  struct { int is_some; Value *inner; } opt;
} Value;

static Value v_int(int x){ Value v={0}; v.kind=VINT; v.i=x; return v; }
static Value v_bool(int x){ Value v={0}; v.kind=VBOOL; v.i=x?1:0; return v; }
static Value v_real(double x){ Value v={0}; v.kind=VREAL; v.f=x; return v; }
static Value v_string(const char *s){ Value v={0}; v.kind=VSTRING; v.s=strdup(s?s:"" ); v.owns=1; _allocs++; return v; }
static Value v_result_ok(const char *text){ Value v={0}; v.kind=VRESULT; v.res.ok=1; v.res.text=strdup(text?text:""); v.owns=1; _allocs++; return v; }
static Value v_result_err(const char *err){ Value v={0}; v.kind=VRESULT; v.res.ok=0; v.res.error=strdup(err?err:""); v.owns=1; _allocs++; return v; }
static Value v_optional_none(void){ Value v={0}; v.kind=VOPTIONAL; v.opt.is_some=0; v.opt.inner=NULL; return v; }
static Value v_copy(Value v);
static Value v_optional_some(Value inner){ Value v={0}; v.kind=VOPTIONAL; v.opt.is_some=1; v.opt.inner=malloc(sizeof(Value)); *v.opt.inner = v_copy(inner); return v; }
static Value v_copy(Value v){
  Value out = v;
  if (v.kind==VSTRING) { out = v_string(v.s); }
  else if (v.kind==VRESULT) { if (v.res.ok) out = v_result_ok(v.res.text); else out = v_result_err(v.res.error);} 
  else if (v.kind==VOPTIONAL) { if (v.opt.is_some && v.opt.inner) out = v_optional_some(*v.opt.inner); else out = v_optional_none(); }
  else if (v.kind==VBOOL) out = v_bool(v.i);
  if (v.ref) {
    out.ref = strdup(v.ref);
    out.ref_interned = 0;
    _allocs++;
  } else {
    out.ref = NULL;
    out.ref_interned = 0;
  }
  return out;
}
static void v_free(Value v){ if (v.ref && !v.ref_interned) { free(v.ref); _frees++; } if (v.kind==VSTRING){ if (v.owns && v.s) { free(v.s); _frees++; } } else if (v.kind==VRESULT){ if (v.res.text){ free(v.res.text); _frees++; } if (v.res.error){ free(v.res.error); _frees++; } } else if (v.kind==VOPTIONAL){ if (v.opt.inner){ v_free(*v.opt.inner); free(v.opt.inner);} } }
static void print_value(FILE *out, Value v){
  switch(v.kind){
  case VINT: fprintf(out, "%d", v.i); break;
  case VBOOL: fprintf(out, "%s", v.i?"True":"False"); break;
  case VREAL: fprintf(out, "%g", v.f); break;
  case VSTRING: fprintf(out, "%s", v.s?v.s:"" ); break;
  case VRESULT:
    if (v.res.ok) fprintf(out, "Ok(%s)", v.res.text ? v.res.text : "");
    else fprintf(out, "Err(%s)", v.res.error ? v.res.error : "");
    break;
  case VOPTIONAL:
    if (v.opt.is_some && v.opt.inner) print_value(out, *v.opt.inner);
    else fprintf(out, "Nothing");
    break;
  }
}

static int is_number(const char *s){ if(!s||!*s) return 0; size_t i=0; if(s[0]=='-'||s[0]=='+') i++; int hasdigit=0; for(;s[i];i++){ if(s[i]>='0'&&s[i]<='9'){hasdigit=1;continue;} if(s[i]=='.') continue; return 0;} return hasdigit; }
static int is_integer(const char *s){ if(!s||!*s) return 0; size_t i=0; if(s[0]=='-'||s[0]=='+') i++; int hasdigit=0; for(;s[i];i++){ if(s[i]>='0'&&s[i]<='9'){hasdigit=1;continue;} return 0;} return hasdigit; }
static Value parse_value(const char *s){ if(is_integer(s)) return v_int(atoi(s)); if(is_number(s)) return v_real(strtod(s,NULL)); return v_string(s); }

typedef struct { char *name; Value val; } Var;
typedef struct Env Env;
typedef struct Env { Var *items; size_t len; size_t cap; Env *parent; char **refs; size_t refs_len; size_t refs_cap; } Env;
static char *env_intern_ref(Env *env, const char *s){
  if (!env || !s) return NULL;
  for(size_t i=0;i<env->refs_len;i++) if (strcmp(env->refs[i], s)==0) return env->refs[i];
  if (env->refs_len==env->refs_cap){ env->refs_cap = env->refs_cap? env->refs_cap*2:8; env->refs=realloc(env->refs, env->refs_cap*sizeof(char*)); }
  char *dup = strdup(s); _allocs++; env->refs[env->refs_len++] = dup; return dup;
}
static Value* env_find(Env *env, const char *name){ for(size_t i=0;i<env->len;i++){ if(strcmp(env->items[i].name,name)==0) return &env->items[i].val;} return NULL; }
static void env_set_raw(Env *env, const char *name, Value vc){
  for(size_t i=0;i<env->len;i++){ if(strcmp(env->items[i].name,name)==0){ v_free(env->items[i].val); env->items[i].val=vc; return; }}
  if(env->len==env->cap){ env->cap=env->cap?env->cap*2:8; env->items=realloc(env->items, env->cap*sizeof(Var)); }
  env->items[env->len].name=strdup(name); env->items[env->len].val=vc; env->len++;
}
static void env_ensure_base_ref(Env *env, const char *name){
  const char *dot = strchr(name, '.');
  if (!dot) return;
  char base[128]; size_t blen = (size_t)(dot - name); if (blen >= sizeof(base)) blen = sizeof(base)-1; strncpy(base, name, blen); base[blen]=0;
  Value *bv = env_find(env, base);
  if (!bv) {
    Value v = v_int(0);
    v.ref = env_intern_ref(env, base);
    v.ref_interned = 1;
    env_set_raw(env, base, v);
  } else if (!bv->ref) {
    bv->ref = env_intern_ref(env, base);
    bv->ref_interned = 1;
  }
}
static void env_set(Env *env, const char *name, Value v){
  if (debug_exec()) fprintf(stderr,"[env_set] %s kind=%d i=%d ref=%s\n", name, v.kind, v.i, v.ref?v.ref:"<null>");
  Value vc = v_copy(v);
  if (v.kind==VSTRING && vc.s && !vc.owns) { vc.s = strdup(vc.s); vc.owns=1; _allocs++; }
  if (vc.ref && strchr(vc.ref, '.')) {
    if (!vc.ref_interned) { free(vc.ref); _frees++; }
    vc.ref = env_intern_ref(env, v.ref);
    vc.ref_interned = 1;
  }
  env_ensure_base_ref(env, name);
  env_set_raw(env, name, vc);
}
static Value env_get_local(Env *env, const char *name){ Value *v = env_find(env,name); if (debug_exec()) fprintf(stderr,"[env_get_local] %s -> %s%s\n", name, v?"hit":"miss", v?"":""); if (v && debug_exec()) fprintf(stderr,"  val kind=%d i=%d ref=%s\n", v->kind, v->i, v->ref?v->ref:"<null>"); if (!v) return v_int(0); return v_copy(*v); }
static Value env_get(Env *env, const char *name){
  Value *vloc = env_find(env, name);
  if (vloc) return v_copy(*vloc);
  const char *dot = strchr(name, '.');
  if (dot) {
    // base name before dot
    char base[128]; size_t blen = (size_t)(dot - name); if (blen >= sizeof(base)) blen = sizeof(base)-1; strncpy(base, name, blen); base[blen]=0;
    Value basev = env_get_local(env, base);
    if (debug_exec()) fprintf(stderr,"[env_get] base=%s ref=%s\n", base, basev.ref?basev.ref:"<null>");
    if (basev.ref) {
      char buf[256]; snprintf(buf,sizeof(buf),"%s%s", basev.ref, dot);
      Value lv2 = env_get_local(env, buf);
      if (!(lv2.kind==VINT && lv2.i==0)) { v_free(basev); return lv2; }
      v_free(lv2);
      if (env->parent) {
        Value pv2 = env_get(env->parent, buf);
        if (!(pv2.kind==VINT && pv2.i==0)) { v_free(basev); return pv2; }
        v_free(pv2);
      }
    }
    v_free(basev);
  }
  if (env->parent) {
    Value pv = env_get(env->parent, name);
    if (env_find(env->parent, name)) return pv;
    v_free(pv);
    if (dot) {
      char base[128]; size_t blen = (size_t)(dot - name); if (blen >= sizeof(base)) blen = sizeof(base)-1; strncpy(base, name, blen); base[blen]=0;
      Value basev = env_get_local(env, base);
      if (basev.ref) {
        char buf[256]; snprintf(buf,sizeof(buf),"%s%s", basev.ref, dot);
        Value pv2 = env_get(env->parent, buf);
        if (env_find(env->parent, buf)) { v_free(basev); return pv2; }
        v_free(pv2);
      }
      v_free(basev);
      for(size_t i=0;i<env->parent->len;i++){
        const char *pname = env->parent->items[i].name;
        const char *pdot = strchr(pname, '.');
        if (!pdot) continue;
        if (strcmp(pdot, dot)==0) return v_copy(env->parent->items[i].val);
      }
    }
  }
  // also suffix fallback in local env
  if (dot) {
    for(size_t i=0;i<env->len;i++){
      const char *pname = env->items[i].name;
      const char *pdot = strchr(pname, '.');
      if (!pdot) continue;
      if (strcmp(pdot, dot)==0) { Value rv=v_copy(env->items[i].val); if (debug_exec()) fprintf(stderr,"[env_get suffix local] %s -> %s\n", name, pname); return rv; }
    }
  }
  if (debug_exec()) fprintf(stderr,"[env_get] %s -> miss\n", name);
  return v_int(0);
}
static void env_free(Env *env){ if (debug_exec()) fprintf(stderr,"[env_free] len=%zu\n", env->len); for(size_t i=0;i<env->len;i++){ if (debug_exec()) fprintf(stderr,"[env_free] %s\n", env->items[i].name); free(env->items[i].name); v_free(env->items[i].val);} free(env->items); for(size_t i=0;i<env->refs_len;i++){ free(env->refs[i]); _frees++; } free(env->refs); }

typedef struct { char *name; size_t idx; } Label;

static long find_label(Label *labels, size_t n, const char *name){ for(size_t i=0;i<n;i++) if(strcmp(labels[i].name,name)==0) return (long)labels[i].idx; return -1; }

static Type *find_schema(const IrProgram *prog, const char *name) {
  for (size_t i=0;i<prog->schemas.len;++i) {
    if (prog->schemas.items[i]->as.schema.name && strcmp(prog->schemas.items[i]->as.schema.name, name)==0)
      return prog->schemas.items[i];
  }
  return NULL;
}

static int validate_json_against_schema(const char *json, Type *schema, char **errmsg);

static const IrFunc *find_func(const IrProgram *prog, const char *name) {
  for (size_t i=0;i<prog->funcs.len;++i) if (strcmp(prog->funcs.items[i].name,name)==0) return &prog->funcs.items[i];
  return NULL;
}

static int execute_func(const IrProgram *prog, const IrFunc *f, Env *env, FILE *in, FILE *out, Oracle *oracle, Value *ret_out){
  // collect labels
  Label *labels=NULL; size_t nlab=0, clab=0;
  for(size_t i=0;i<f->instrs.len;i++) if(f->instrs.items[i].op==IR_LABEL){ if(nlab==clab){ clab=clab?clab*2:8; labels=realloc(labels, clab*sizeof(Label)); } labels[nlab].name=f->instrs.items[i].s; labels[nlab].idx=i; nlab++; }
  // temps
  size_t maxt= f->next_temp + 16; Value *temps = calloc(maxt, sizeof(Value));
  for(size_t i=0;i<maxt;i++) temps[i]=v_int(0);
  int had_ret=0; Value retval=v_int(0);
  size_t ip=0;
  while(ip < f->instrs.len){
    const IrInstr *ins = &f->instrs.items[ip];
    switch(ins->op){
    case IR_CONST_INT: v_free(temps[ins->dest]); temps[ins->dest]=v_int(ins->arg1); break;
    case IR_CONST_BOOL: v_free(temps[ins->dest]); temps[ins->dest]=v_bool(ins->arg1); break;
    case IR_CONST_REAL: v_free(temps[ins->dest]); temps[ins->dest]=v_real(ins->f); break;
    case IR_CONST_STRING: v_free(temps[ins->dest]); temps[ins->dest]=v_string(ins->s?ins->s:""); break;
    case IR_CONST_OPTIONAL_NONE: v_free(temps[ins->dest]); temps[ins->dest]=v_optional_none(); break;
    case IR_LOAD_VAR: v_free(temps[ins->dest]); temps[ins->dest]=env_get(env, ins->s); if (temps[ins->dest].ref) { free(temps[ins->dest].ref); _frees++; } temps[ins->dest].ref=strdup(ins->s); temps[ins->dest].ref_interned=0; _allocs++; break;
    case IR_STORE_VAR: env_set(env, ins->s, temps[ins->arg1]); break;
    case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD: {
      Value a=temps[ins->arg1], b=temps[ins->arg2];
      if (ins->op==IR_ADD && (a.kind==VSTRING || b.kind==VSTRING)) {
        char buf_a[64], buf_b[64];
        const char *sa = (a.kind==VSTRING)? (a.s?a.s:"") : (snprintf(buf_a,sizeof(buf_a),"%g", (a.kind==VREAL)?a.f:(double)a.i), buf_a);
        const char *sb = (b.kind==VSTRING)? (b.s?b.s:"") : (snprintf(buf_b,sizeof(buf_b),"%g", (b.kind==VREAL)?b.f:(double)b.i), buf_b);
        size_t lena=strlen(sa), lenb=strlen(sb);
        char *res=malloc(lena+lenb+1); memcpy(res, sa, lena); memcpy(res+lena, sb, lenb); res[lena+lenb]='\0';
        v_free(temps[ins->dest]); temps[ins->dest]=v_string(res); free(res);
        break;
      }
      double da=(a.kind==VREAL)?a.f:a.i; double db=(b.kind==VREAL)?b.f:b.i;
      double r=0; switch(ins->op){ case IR_ADD:r=da+db;break; case IR_SUB:r=da-db;break; case IR_MUL:r=da*db;break; case IR_DIV:r=db!=0?da/db:0;break; case IR_MOD:r=(int)da % (int)db;break; default:break; }
      int any_real = (a.kind==VREAL || b.kind==VREAL);
      v_free(temps[ins->dest]); temps[ins->dest]= any_real ? v_real(r) : v_int((int)r);
      break; }
    case IR_EQ: case IR_NEQ: case IR_LT: case IR_GT: case IR_LE: case IR_GE: {
      Value a=temps[ins->arg1], b=temps[ins->arg2]; double da=(a.kind==VREAL)?a.f:a.i; double db=(b.kind==VREAL)?b.f:b.i; int res=0;
      switch(ins->op){ case IR_EQ: res = (da==db); break; case IR_NEQ: res=(da!=db); break; case IR_LT: res=(da<db); break; case IR_GT: res=(da>db); break; case IR_LE: res=(da<=db); break; case IR_GE: res=(da>=db); break; default: break; }
      v_free(temps[ins->dest]); temps[ins->dest]=v_bool(res); break; }
    case IR_AND: case IR_OR: {
      Value a=temps[ins->arg1], b=temps[ins->arg2];
      int ta = (a.kind==VINT||a.kind==VREAL||a.kind==VBOOL) ? ((a.kind==VREAL)?(a.f!=0):a.i!=0) : (a.kind==VSTRING? (a.s && a.s[0]):0);
      int tb = (b.kind==VINT||b.kind==VREAL||b.kind==VBOOL) ? ((b.kind==VREAL)?(b.f!=0):b.i!=0) : (b.kind==VSTRING? (b.s && b.s[0]):0);
      int res = (ins->op==IR_AND) ? (ta && tb) : (ta || tb);
      v_free(temps[ins->dest]); temps[ins->dest]=v_bool(res); break; }
    case IR_JUMP: {
      long idx = find_label(labels, nlab, ins->s);
      if(idx>=0){ ip = (size_t)idx; continue; }
      break; }
    case IR_JUMP_IF_FALSE: {
      Value c = temps[ins->arg1]; int truthy=0;
      if(c.kind==VINT) truthy = c.i!=0; else if(c.kind==VREAL) truthy = c.f!=0; else if (c.kind==VBOOL) truthy = c.i!=0; else truthy = c.s && c.s[0];
      if(!truthy){ long idx=find_label(labels,nlab,ins->s); if(idx>=0){ ip=(size_t)idx; continue; }}
      break; }
    case IR_LABEL: break;
    case IR_RET:
      if (ret_out) { v_free(retval); retval = v_copy(temps[ins->arg1]); had_ret=1; }
      goto done;
    case IR_PRINT: print_value(out, temps[ins->arg1]); fflush(out); break;
    case IR_PRINTLN: if(ins->arg1>=0) print_value(out, temps[ins->arg1]); fputc('\n', out); fflush(out); break;
    case IR_READLN: {
      char *line=NULL; size_t n=0; ssize_t r=getline(&line, &n, in);
      if(r>0 && line[r-1]=='\n') line[r-1]='\0';
      Value v = parse_value(line?line:"" ); env_set(env, ins->s, v); v_free(v); free(line);
      break; }
    case IR_READ_FILE: {
      Value pathv = temps[ins->arg1]; const char *path = (pathv.kind==VSTRING && pathv.s)?pathv.s:"";
      FILE *fpy = fopen(path, "rb"); if(!fpy){ v_free(temps[ins->dest]); temps[ins->dest]=v_string(""); break; }
      fseek(fpy,0,SEEK_END); long len=ftell(fpy); rewind(fpy);
      char *buf = malloc(len+1); if(!buf){ fclose(fpy); v_free(temps[ins->dest]); temps[ins->dest]=v_string(""); break; }
      size_t read_n = fread(buf,1,(size_t)len,fpy);
      buf[read_n]='\0';
      fclose(fpy);
      v_free(temps[ins->dest]); temps[ins->dest]=v_string(buf); free(buf);
      break; }
    case IR_WRITE_FILE: {
      Value pathv = temps[ins->arg1]; Value contentv = temps[ins->arg2];
      const char *path = (pathv.kind==VSTRING && pathv.s)?pathv.s:"";
      const char *content = (contentv.kind==VSTRING && contentv.s)?contentv.s:"";
      FILE *fpy = fopen(path, "wb"); if(fpy){ fwrite(content,1,strlen(content),fpy); fclose(fpy);} break; }
    case IR_ASK: {
      Value pv = temps[ins->arg1];
      const char *prompt = (pv.kind==VSTRING && pv.s)?pv.s:"";
      OracleResult r = oracle_call_text(oracle, prompt);
      v_free(temps[ins->dest]);
      if (r.ok) {
        if (ins->s2) {
          Type *schema = find_schema(prog, ins->s2);
          char *errmsg=NULL;
          int valid = schema ? validate_json_against_schema(r.text ? r.text : "", schema, &errmsg) : 0;
          if (debug_exec()) {
            fprintf(stderr, "[exec] ask schema=%s found=%s valid=%d err=%s (schemas len=%zu)\n", ins->s2, schema?"yes":"no", valid, errmsg?errmsg:"(null)", prog->schemas.len);
            for (size_t ii=0; ii<prog->schemas.len; ++ii) {
              fprintf(stderr, "[exec] schema[%zu]=%s\n", ii, prog->schemas.items[ii]->as.schema.name ? prog->schemas.items[ii]->as.schema.name : "(null)");
            }
          }
          if (schema && valid) {
            temps[ins->dest] = v_result_ok(r.text ? r.text : "");
          } else {
            temps[ins->dest] = v_result_err(errmsg ? errmsg : (schema?"extraction failed":"schema not found"));
          }
          free(errmsg);
        } else {
          temps[ins->dest] = v_result_ok(r.text ? r.text : "");
        }
      } else {
        if (ins->arg2 >= 0) {
          Value fb = temps[ins->arg2];
          if (fb.kind == VSTRING) temps[ins->dest] = v_result_ok(fb.s ? fb.s : "");
          else if (fb.kind == VRESULT && fb.res.ok) temps[ins->dest] = v_result_ok(fb.res.text ? fb.res.text : "");
          else temps[ins->dest] = v_result_ok("");
        } else {
          temps[ins->dest] = v_result_err(r.error ? r.error : "oracle error");
        }
      }
      oracle_result_free(r);
      break; }
    case IR_RESULT_UNWRAP: {
      Value rv = temps[ins->arg1];
      Value fb = ins->arg2 >=0 ? temps[ins->arg2] : v_int(0);
      v_free(temps[ins->dest]);
      if (rv.kind == VRESULT) {
        if (rv.res.ok) {
          temps[ins->dest] = v_string(rv.res.text ? rv.res.text : "");
        } else {
          if (ins->arg2 >=0 && fb.kind==VSTRING) temps[ins->dest] = v_string(fb.s ? fb.s : "");
          else temps[ins->dest] = v_string("");
        }
      } else if (rv.kind == VSTRING) {
        temps[ins->dest] = v_string(rv.s ? rv.s : "");
      } else {
        temps[ins->dest] = v_string("");
      }
      break; }
    case IR_RESULT_IS_OK: {
      Value rv = temps[ins->arg1];
      v_free(temps[ins->dest]);
      if (rv.kind == VRESULT) temps[ins->dest] = v_int(rv.res.ok ? 1 : 0);
      else temps[ins->dest] = v_int(0);
      break; }
    case IR_RESULT_UNWRAP_ERR: {
      Value rv = temps[ins->arg1];
      v_free(temps[ins->dest]);
      if (rv.kind == VRESULT) {
        if (rv.res.ok) temps[ins->dest] = v_string("");
        else temps[ins->dest] = v_string(rv.res.error ? rv.res.error : "");
      } else {
        temps[ins->dest] = v_string("");
      }
      break; }
    case IR_MAKE_RESULT_OK: {
      Value rv = temps[ins->arg1];
      v_free(temps[ins->dest]);
      char buf[64]; const char *s = NULL; char *tmp=NULL;
      if (rv.kind==VSTRING) s = rv.s ? rv.s : "";
      else if (rv.kind==VINT) { snprintf(buf,sizeof(buf), "%d", rv.i); tmp=strdup(buf); s=tmp; }
      else if (rv.kind==VREAL) { snprintf(buf,sizeof(buf), "%g", rv.f); tmp=strdup(buf); s=tmp; }
      else if (rv.kind==VBOOL) { tmp=strdup(rv.i?"True":"False"); s=tmp; }
      else s = "";
      temps[ins->dest] = v_result_ok(s);
      if (tmp) free(tmp);
      break; }
    case IR_MAKE_RESULT_ERR: {
      Value rv = temps[ins->arg1];
      v_free(temps[ins->dest]);
      char buf[64]; const char *s = NULL; char *tmp=NULL;
      if (rv.kind==VSTRING) s = rv.s ? rv.s : "";
      else if (rv.kind==VINT) { snprintf(buf,sizeof(buf), "%d", rv.i); tmp=strdup(buf); s=tmp; }
      else if (rv.kind==VREAL) { snprintf(buf,sizeof(buf), "%g", rv.f); tmp=strdup(buf); s=tmp; }
      else if (rv.kind==VBOOL) { tmp=strdup(rv.i?"True":"False"); s=tmp; }
      else s = "";
      temps[ins->dest] = v_result_err(s);
      if (tmp) free(tmp);
      break; }
    case IR_CONCAT: {
      Value a = temps[ins->arg1];
      Value b = temps[ins->arg2];
      char buf[64];
      const char *sa=NULL, *sb=NULL;
      char *sa_tmp=NULL, *sb_tmp=NULL;
      if (a.kind==VSTRING) sa = a.s ? a.s : "";
      else if (a.kind==VINT) { snprintf(buf,sizeof(buf),"%d",a.i); sa_tmp=strdup(buf); sa=sa_tmp; }
      else if (a.kind==VREAL) { snprintf(buf,sizeof(buf),"%g",a.f); sa_tmp=strdup(buf); sa=sa_tmp; }
      else if (a.kind==VBOOL) { sa_tmp=strdup(a.i?"True":"False"); sa=sa_tmp; }
      else if (a.kind==VRESULT) sa = a.res.ok && a.res.text ? a.res.text : (a.res.error ? a.res.error : "");
      else sa = "";
      if (b.kind==VSTRING) sb = b.s ? b.s : "";
      else if (b.kind==VINT) { snprintf(buf,sizeof(buf),"%d",b.i); sb_tmp=strdup(buf); sb=sb_tmp; }
      else if (b.kind==VREAL) { snprintf(buf,sizeof(buf),"%g",b.f); sb_tmp=strdup(buf); sb=sb_tmp; }
      else if (b.kind==VBOOL) { sb_tmp=strdup(b.i?"True":"False"); sb=sb_tmp; }
      else if (b.kind==VRESULT) sb = b.res.ok && b.res.text ? b.res.text : (b.res.error ? b.res.error : "");
      else sb = "";
      size_t lena=strlen(sa), lenb=strlen(sb);
      char *res = malloc(lena+lenb+1);
      memcpy(res, sa, lena); memcpy(res+lena, sb, lenb); res[lena+lenb]='\0';
      v_free(temps[ins->dest]);
      temps[ins->dest] = v_string(res);
      if (sa_tmp) free(sa_tmp);
      if (sb_tmp) free(sb_tmp);
      free(res);
      break; }
    case IR_RESULT_OR_FALLBACK: {
      Value rv = temps[ins->arg1];
      Value fb = ins->arg2 >=0 ? temps[ins->arg2] : v_int(0);
      v_free(temps[ins->dest]);
      if (rv.kind == VRESULT) {
        if (rv.res.ok) temps[ins->dest] = v_result_ok(rv.res.text ? rv.res.text : "");
        else {
          if (ins->arg2 >=0 && fb.kind==VSTRING) temps[ins->dest] = v_result_ok(fb.s ? fb.s : "");
          else temps[ins->dest] = v_result_err(rv.res.error ? rv.res.error : "");
        }
      } else if (rv.kind == VSTRING) {
        temps[ins->dest] = v_result_ok(rv.s ? rv.s : "");
      } else {
        temps[ins->dest] = v_result_err("invalid result");
      }
      break; }
    case IR_CALL: {
      const IrFunc *cf = find_func(prog, ins->s ? ins->s : "");
      Value rv = v_int(0);
      if (cf) {
        Env newenv={0}; newenv.parent = env;
        if (cf->param_count >0 && ins->arg1>=0) { env_set(&newenv, cf->params[0], temps[ins->arg1]); }
        if (cf->param_count >1 && ins->arg2>=0) { env_set(&newenv, cf->params[1], temps[ins->arg2]); }
        execute_func(prog, cf, &newenv, in, out, oracle, &rv);
        env_free(&newenv);
      }
      v_free(temps[ins->dest]);
      temps[ins->dest] = v_copy(rv);
      v_free(rv);
      break; }
    case IR_INDEX: {
      Value idxv = temps[ins->arg2]; int idx = (idxv.kind==VREAL)?(int)idxv.f: idxv.i;
      // fallback: env lookup base.idx
      if (ins->s) {
        char buf[256]; snprintf(buf,sizeof(buf),"%s.%d", ins->s, idx);
        v_free(temps[ins->dest]); temps[ins->dest]=env_get(env, buf);
        if (temps[ins->dest].ref) { free(temps[ins->dest].ref); _frees++; }
        temps[ins->dest].ref = strdup(buf);
        temps[ins->dest].ref_interned = 0;
        _allocs++;
      } else {
        v_free(temps[ins->dest]); temps[ins->dest]=v_int(0);
      }
      break; }
    default: break;
    }
    ip++;
  }

done:
  if (labels) free(labels);
  labels = NULL;
  if (debug_exec() && f && f->name && strcmp(f->name,"Average")==0) {
    Value vt=env_get(env,"Total"); Value vc=env_get(env,"Count"); Value vr=env_get(env,"Result");
    fprintf(stderr,"[exec] Average Total=%d Count=%d Result kind=%d i=%d\n", vt.i, vc.i, vr.kind, vr.i);
    v_free(vt); v_free(vc); v_free(vr);
  }
  if (ret_out) {
    if (!had_ret) {
      Value rv = env_get(env, "Result");
      v_free(retval);
      retval = rv;
    }
    *ret_out = retval;
  } else {
    v_free(retval);
  }
  for(size_t i=0;i<maxt;i++) v_free(temps[i]);
  free(temps);
  return 0;
}

static int validate_json_against_schema(const char *json, Type *schema, char **errmsg) {
  // Minimal validator: object with string keys, values string/number/bool. No nesting/arrays.
  const char *p = json;
  while (*p && isspace((unsigned char)*p)) p++;
  if (*p != '{') { if (errmsg) *errmsg = strdup("expected object"); return 0; }
  p++;
  // parse fields
  typedef struct { char *key; char *val; int is_string; int is_bool; int is_number; } Field;
  Field *fields=NULL; size_t len=0, cap=0;
  while (*p) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '}') { p++; break; }
    if (*p != '"') { if (errmsg) *errmsg = strdup("expected field name"); goto fail; }
    p++; const char *kstart=p; while (*p && *p!='"') p++; if (*p!='"') { if(errmsg)*errmsg=strdup("unterminated field name"); goto fail; }
    size_t klen = p-kstart; char *key = strndup(kstart, klen); p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != ':') { if(errmsg)*errmsg=strdup("expected colon"); free(key); goto fail; }
    p++; while (*p && isspace((unsigned char)*p)) p++;
    Field f={0}; f.key=key;
    if (*p=='"') { p++; const char *vstart=p; while(*p && *p!='"') p++; if(*p!='"'){ if(errmsg)*errmsg=strdup("unterminated string"); free(key); goto fail;} size_t vlen=p-vstart; f.val=strndup(vstart,vlen); f.is_string=1; p++; }
    else if (strncmp(p,"true",4)==0){ f.val=strdup("true"); f.is_bool=1; p+=4; }
    else if (strncmp(p,"false",5)==0){ f.val=strdup("false"); f.is_bool=1; p+=5; }
    else { const char *vstart=p; if(*p=='-'||*p=='+') p++; while(*p && (isdigit((unsigned char)*p)||*p=='.')) p++; size_t vlen=p-vstart; if(vlen==0){ if(errmsg)*errmsg=strdup("expected value"); free(key); goto fail;} f.val=strndup(vstart,vlen); f.is_number=1; }
    if (len==cap){ cap=cap?cap*2:4; fields=realloc(fields,cap*sizeof(Field)); }
    fields[len++]=f;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p==','){ p++; continue; }
    if (*p=='}'){ p++; break; }
  }
  // validate schema fields
  for (size_t i=0;i<schema->as.schema.len;++i){ SchemaField *sf=&schema->as.schema.items[i]; Field *ff=NULL; for(size_t j=0;j<len;++j){ if(strcmp(fields[j].key,sf->name)==0){ ff=&fields[j]; break; } } if(!ff){ if(errmsg){ size_t n=strlen(sf->name)+32; *errmsg=malloc(n); snprintf(*errmsg,n,"missing field %s",sf->name);} goto fail; }
    Type *ft=sf->type; if(ft->kind==TYPEK_STRING){ if(!ff->is_string){ if(errmsg){size_t n=strlen(sf->name)+32; *errmsg=malloc(n); snprintf(*errmsg,n,"field %s not string",sf->name);} goto fail;} }
    else if(ft->kind==TYPEK_INT){ if(!ff->is_number||strchr(ff->val,'.')){ if(errmsg){size_t n=strlen(sf->name)+32; *errmsg=malloc(n); snprintf(*errmsg,n,"field %s not integer",sf->name);} goto fail;} }
    else if(ft->kind==TYPEK_REAL){ if(!ff->is_number){ if(errmsg){size_t n=strlen(sf->name)+32; *errmsg=malloc(n); snprintf(*errmsg,n,"field %s not number",sf->name);} goto fail;} }
    else if(ft->kind==TYPEK_BOOL){ if(!ff->is_bool){ if(errmsg){size_t n=strlen(sf->name)+32; *errmsg=malloc(n); snprintf(*errmsg,n,"field %s not bool",sf->name);} goto fail;} }
  }
  for(size_t j=0;j<len;++j){ free(fields[j].key); free(fields[j].val);} free(fields); return 1;
fail:
  if(fields){ for(size_t j=0;j<len;++j){ free(fields[j].key); free(fields[j].val);} free(fields);} return 0; }

int ir_execute(const IrProgram *prog, FILE *in, FILE *out, Oracle *oracle){ if(!prog||prog->funcs.len==0) return 1; Env env={0}; int rc= execute_func(prog, &prog->funcs.items[0], &env, in, out, oracle, NULL); env_free(&env); if (debug_exec()) fprintf(stderr,"[allocs] allocs=%zu frees=%zu\n", _allocs, _frees); return rc; }

static char *read_file(const char *path, size_t *len_out){ FILE *f=fopen(path, "rb"); if(!f) return NULL; fseek(f,0,SEEK_END); long len=ftell(f); rewind(f); char *buf=malloc(len+1); size_t read_n=fread(buf,1,(size_t)len,f); buf[read_n]='\0'; fclose(f); if(len_out) *len_out=read_n; return buf; }

static Oracle *g_oracle = NULL;
void exec_set_global_oracle(Oracle *o){ g_oracle = o; }

int liminal_run_file_streams(const char *path, FILE *in, FILE *out){ size_t len=0; char *src = read_file(path, &len); if(!src){ fprintf(stderr, "Unable to read %s\n", path); return 1; }
  if (debug_exec()) fprintf(stderr, "[exec] read file ok len=%zu\n", len);
  Parser *p = parser_create(src, len); ASTNode *ast = parse_program(p);
  if (debug_exec()) fprintf(stderr, "[exec] parse done\n");
  TypeCheckResult tcr = typecheck_program(ast);
  if (debug_exec()) fprintf(stderr, "[exec] typecheck ok=%d\n", tcr.ok ? 1 : 0);
  if(!tcr.ok){ for(size_t i=0;i<tcr.errors.len;i++){ fprintf(stderr, "Type error: %s\n", tcr.errors.items[i].message); } typecheck_result_free(&tcr); ast_free(ast); parser_destroy(p); free(src); return 1; }
  typecheck_result_free(&tcr);
  IrProgram *ir = ir_from_ast(ast);
  if (debug_exec()) fprintf(stderr, "[exec] ir_from_ast done\n");
  char *errmsg=NULL; if(!ir_validate(ir,&errmsg)){ fprintf(stderr, "IR invalid: %s\n", errmsg?errmsg:""); free(errmsg); ir_program_free(ir); ast_free(ast); parser_destroy(p); free(src); return 1; }
  if (debug_exec()) fprintf(stderr, "[exec] ir validated\n");
  const char *dbg = getenv("LIMINAL_DEBUG_IR");
  if (dbg) {
    char *irstr = ir_program_print(ir);
    fprintf(stderr, "IR:\n%s\n", irstr);
    free(irstr);
  }
  if (debug_exec()) fprintf(stderr, "[exec] executing\n");
  Oracle *oracle = g_oracle ? g_oracle : oracle_from_env();
  int rc = ir_execute(ir, in?in:stdin, out?out:stdout, oracle);
  if (!g_oracle && oracle) oracle_free(oracle);
  if (debug_exec()) fprintf(stderr, "[exec] done rc=%d\n", rc);
  ir_program_free(ir); ast_free(ast); parser_destroy(p); free(src); return rc; }

int liminal_run_file(const char *path){ return liminal_run_file_streams(path, stdin, stdout); }
