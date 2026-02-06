#ifndef LIMINAL_EXEC_H
#define LIMINAL_EXEC_H

#include <stdio.h>
#include "liminal/ir.h"
#include "liminal/oracles.h"

#ifdef __cplusplus
extern "C" {
#endif

int ir_execute(const IrProgram *prog, FILE *in, FILE *out, struct Oracle *oracle);
int liminal_run_file_streams(const char *path, FILE *in, FILE *out);
int liminal_run_file(const char *path);
void exec_set_global_oracle(struct Oracle *o);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_EXEC_H
