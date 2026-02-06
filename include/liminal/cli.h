#ifndef LIMINAL_CLI_H
#define LIMINAL_CLI_H

#include <stdio.h>

#include "liminal/version.h"

#ifdef __cplusplus
extern "C" {
#endif

int liminal_main(int argc, char **argv);
const char *liminal_help_text(void);
const char *liminal_version_text(void);

#ifdef __cplusplus
}
#endif

#endif // LIMINAL_CLI_H
