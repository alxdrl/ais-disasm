#include <stdint.h>

#include "ais-print.h"
#include "hashtab.h"

typedef uint32_t ais_vma;

extern hashtab_t *s_table;

#define TIC6X_VMA_FMT "l"
#define tic6x_sprintf_vma(s,x) sprintf (s, "%08" TIC6X_VMA_FMT "x", x)

char *
tic6x_get_symbol(ais_vma addr)
{
	return (char *)ht_search(s_table, &addr, sizeof(addr));
}

void
tic6x_print_label(bfd_vma addr, char *buf)
{
       char *symbol = tic6x_get_symbol(addr);
       buf[0] = '\0';
       if (symbol != NULL) {
               sprintf(buf, "%s:", symbol);
       }
}

void
tic6x_print_address(bfd_vma addr, struct disassemble_info *info)
{
  char *sym = tic6x_get_symbol(addr);
  if (sym == NULL) {
	  char buf[30];
	  tic6x_sprintf_vma (buf, addr);
	  (*info->fprintf_func) (info->stream, "0x%s", buf);
  } else {
	  (*info->fprintf_func) (info->stream, "%s", sym);
  }
}

int
tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info)
{
	unsigned int word;
	buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
	char *sym = tic6x_get_symbol(word);
	if (sym == NULL) {
		(*info->fprintf_func) (info->stream, ".word 0x%08x", word);
	} else {
		(*info->fprintf_func) (info->stream, ".word %s", sym);
	} 
	return 4;
}

int
tic6x_section_print_string(bfd_vma addr, struct disassemble_info *info)
{
    int count = 0;
    bfd_byte c;
    info->fprintf_func(info->stream, ".string '");
    buffer_read_memory (addr++, &c, 1, info);
    count++;
    while (c != 0) {
        if (c == '\n')
            info->fprintf_func(info->stream, "\\n");
        else if (c == '\r')
            info->fprintf_func(info->stream, "\\r");
        else
            info->fprintf_func(info->stream, "%c", c);
        buffer_read_memory (addr++, &c, 1, info);
        count++;
    }
    info->fprintf_func(info->stream, "',0");
    return count;
}
