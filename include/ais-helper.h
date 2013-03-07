#ifndef __AIS_HELPER_H
#   define __AIS_HELPER_H

#include <stddef.h>
#include <libiberty/ansidecl.h>

/* Pseudo FILE object for strings.  */
typedef struct
{
  char *buffer;
  size_t pos;
  size_t alloc;
} SFILE;

#define SFILE_ALLOC_DEFAULT 120

/* alloc SFILE buffer */
void alloc_buffer(SFILE *f);

/* sprintf to a "stream".  */
int ATTRIBUTE_PRINTF_2
disasm_sprintf (SFILE *f, const char *format, ...);

#endif
