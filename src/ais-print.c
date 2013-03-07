#include "ais-print.h"

#define TIC6X_VMA_FMT "l"
#define tic6x_sprintf_vma(s,x) sprintf (s, "%08" TIC6X_VMA_FMT "x", x)

void
tic6x_print_address(bfd_vma addr, struct disassemble_info *info)
{
  char buf[30];

  tic6x_sprintf_vma (buf, addr);
  (*info->fprintf_func) (info->stream, "0x%s", buf);
}

int
tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info)
{
     unsigned int word;
     buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
     info->fprintf_func(info->stream, ".word 0x%08x", word);
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
