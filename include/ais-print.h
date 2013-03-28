#ifndef __AIS_PRINT_H
#   define __AIS_PRINT_H

#include "ais-version.h"

#include <dis-asm.h>

#include "ais.h"

typedef int (*tic6x_print_region_ftype) (bfd_vma addr, struct disassemble_info *info);

char *tic6x_get_symbol(ais_vma addr);
void tic6x_print_label(bfd_vma addr, char *buf);
void tic6x_print_address(bfd_vma addr, struct disassemble_info *info);
int tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info);
int tic6x_section_print_string(bfd_vma addr, struct disassemble_info *info);
int tic6x_section_print_mixed(bfd_vma addr, struct disassemble_info *info);

#endif
