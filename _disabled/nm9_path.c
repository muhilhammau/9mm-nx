/* nm9_path.c -- port_path_fixup: resolve "." and ".." in the SD path.
 * The engine builds "<root>/./textures/X.tga" and "<root>/../textures"; the
 * runtime only collapses duplicate separators and turns '\' into '/', and
 * Horizon's file system takes the other components literally. MIT. */
#include <stddef.h>
#include <string.h>

const char *port_path_fixup(const char *real, char *out, size_t cap) {
  char tmp[512];
  if (!real || !out || !cap)
    return real;
  size_t n = strlen(real);
  if (n >= sizeof tmp || n >= cap)
    return real;
  /* only a path with a dot segment is touched */
  if (!strstr(real, "/./") && !strstr(real, "/../") && !(n >= 2 && !strcmp(real + n - 2, "/.")) &&
      !(n >= 3 && !strcmp(real + n - 3, "/..")))
    return real;
  memcpy(tmp, real, n + 1); /* real may be out itself */

  const char *p = tmp;
  size_t o = 0;
  const char *colon = strchr(tmp, ':');
  const char *slash = strchr(tmp, '/');
  if (colon && (!slash || colon < slash)) { /* "sdmc:" */
    o = (size_t)(colon + 1 - tmp);
    memcpy(out, tmp, o);
    p = colon + 1;
  }
  if (*p == '/')
    out[o++] = '/';
  size_t root = o;
  while (*p) {
    while (*p == '/')
      p++;
    if (!*p)
      break;
    const char *seg = p;
    while (*p && *p != '/')
      p++;
    size_t len = (size_t)(p - seg);
    if (len == 1 && seg[0] == '.')
      continue;
    if (len == 2 && seg[0] == '.' && seg[1] == '.') {
      if (o > root) {
        o--; /* the '/' after the last segment */
        while (o > root && out[o - 1] != '/')
          o--;
      }
      continue;
    }
    memcpy(out + o, seg, len);
    o += len;
    out[o++] = '/';
  }
  if (o > root)
    o--; /* drop the trailing '/' */
  out[o] = 0;
  return out;
}
