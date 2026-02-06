#include "liminal/cli.h"
#include "liminal/exec.h"
#include <string.h>

static const char *HELP_TEXT =
    "liminal - Liminal Programming Language Compiler\n"
    "\n"
    "Usage:\n"
    "  liminal [--help] [--version]\n"
    "  liminal run <file>\n"
    "\n"
    "Options:\n"
    "  --help, -h      Show this help message\n"
    "  --version, -v   Show version information\n";

const char *liminal_help_text(void) {
  return HELP_TEXT;
}

const char *liminal_version_text(void) {
  return "liminal " LIMINAL_VERSION;
}

static int print_help(void) {
  fputs(liminal_help_text(), stdout);
  return 0;
}

static int print_version(void) {
  fputs(liminal_version_text(), stdout);
  fputc('\n', stdout);
  return 0;
}

int liminal_main(int argc, char **argv) {
  if (argc <= 1) {
    // default: show help
    return print_help();
  }

  if (argc == 2) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      return print_help();
    }
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
      return print_version();
    }
  }

  if (argc >= 3 && strcmp(argv[1], "run") == 0) {
    return liminal_run_file(argv[2]);
  }

  fprintf(stderr, "Unknown option: %s\n", argv[1]);
  fprintf(stderr, "Run 'liminal --help' for usage.\n");
  return 1;
}
