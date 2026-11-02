/* nm9_stubs.c -- shims for imports the runtime has no shim for.
 *
 * ptsname/unlockpt: pseudo-terminal helpers pulled in by the library (no ptys
 * on Horizon). Failing is the honest answer: ptsname() -> NULL, unlockpt() -> -1.
 *
 * gen_imports.py finds a shim by its b_<symbol> name.
 * MIT.
 */
#include <stddef.h>

char *b_ptsname(int fd) {
  (void)fd;
  return NULL;
}

int b_unlockpt(int fd) {
  (void)fd;
  return -1;
}
