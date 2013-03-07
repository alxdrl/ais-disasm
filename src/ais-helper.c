#include <stdarg.h>
#include <stdio.h>
#include <libiberty/libiberty.h>

#include "ais-helper.h"

/* alloc SFILE buffer */
void alloc_buffer(SFILE *f) {
    if (f->buffer == NULL) {
        f->alloc = SFILE_ALLOC_DEFAULT;
        f->buffer = (void *) xmalloc (f->alloc);
    }
}

/* sprintf to a "stream".  */
int ATTRIBUTE_PRINTF_2
disasm_sprintf (SFILE *f, const char *format, ...)
{
  size_t n;
  va_list args;

  while (1)
    {
      size_t space = f->alloc - f->pos;

      va_start (args, format);
      n = vsnprintf (f->buffer + f->pos, space, format, args);
      va_end (args);

      if (space > n)
        break;

      f->alloc = (f->alloc + n) * 2;
      f->buffer = (void *) xrealloc (f->buffer, f->alloc);
    }
  f->pos += n;

  return n;
}
