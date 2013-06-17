#include "ais-print.h"
#include "ais-symbol.h"
#include "hashtab.h"

extern hashtab_t *s_table;

#define TIC6X_VMA_FMT "l"
#define tic6x_sprintf_vma(s,x) sprintf (s, "%08" TIC6X_VMA_FMT "x", x)

static ais_vma cached_address;
static ais_symbol_t *cached_sym;

char *
tic6x_get_cached_symbol_name(ais_vma addr)
{
	ais_symbol_t *sym = cached_sym;
	cached_sym = NULL;
	if (cached_address == addr && cached_sym != NULL)
		return cached_sym->name;
	cached_address = 0;
	sym = (ais_symbol_t *)ht_search(s_table, &addr, sizeof(addr));
	cached_sym = sym;
	cached_address = addr;
	if (sym == NULL)
		return NULL;
	return cached_sym->name;
}

char *
tic6x_get_symbol_name(ais_vma addr)
{
	ais_symbol_t *sym = (ais_symbol_t *)ht_search(s_table, &addr, sizeof(addr));
	if (sym == NULL)
		return NULL;
	return sym->name;
}

ais_symbol_t *
tic6x_get_symbol(ais_vma addr)
{
	return (ais_symbol_t *)ht_search(s_table, &addr, sizeof(addr));
}

void
tic6x_print_label(ais_symbol_t *symbol, char *buf)
{
       buf[0] = '\0';
       if (symbol && symbol->name) {
               sprintf(buf, "%s", symbol->name);
       }
}

void
tic6x_print_address(bfd_vma addr, struct disassemble_info *info)
{
  char *sym = tic6x_get_symbol_name(addr);
  if (sym == NULL) {
	  char buf[30];
	  tic6x_sprintf_vma (buf, addr);
	  (*info->fprintf_func) (info->stream, "0x%s", buf);
  } else {
	  (*info->fprintf_func) (info->stream, "%s", sym);
  }
}

int
tic6x_section_print_byte(bfd_vma addr, struct disassemble_info *info)
{
	bfd_byte byte;
	buffer_read_memory (addr, (bfd_byte *)&byte, 1, info);
	(*info->fprintf_func) (info->stream, ".byte 0x%02x", byte);
	return 1;
}

int
tic6x_section_print_short(bfd_vma addr, struct disassemble_info *info)
{
	unsigned short hword;
	if (addr & 1)
		return tic6x_section_print_byte(addr, info);
	if (tic6x_get_symbol(addr + 1))
		return tic6x_section_print_byte(addr, info);
	buffer_read_memory (addr, (bfd_byte *)&hword, 2, info);
	(*info->fprintf_func) (info->stream, ".short 0x%04x", hword);
	return 2;
}

int
tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info)
{
	unsigned int word;
	switch (addr & 3) {
	case 1:
	case 3:
		return tic6x_section_print_byte(addr, info);
	case 2:
		return tic6x_section_print_short(addr, info);
	}
	if (tic6x_get_symbol(addr + 1))
		return tic6x_section_print_byte(addr, info);
	if (tic6x_get_symbol(addr + 2))
		return tic6x_section_print_short(addr, info);
	if (tic6x_get_symbol(addr + 3))
		return tic6x_section_print_short(addr, info);
	buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
	buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
	char *sym = tic6x_get_symbol_name(word);
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
        else if (c == '\t')
            info->fprintf_func(info->stream, "\\t");
        else
            info->fprintf_func(info->stream, "%c", c);
        buffer_read_memory (addr++, &c, 1, info);
        count++;
	if (tic6x_get_symbol(addr)) break;
    }
    if (c == 0)
	   info->fprintf_func(info->stream, "',0");
    else
	   info->fprintf_func(info->stream, "%c'", c);
    return count;
}

#define MAX_STR 64
#define PRINT_MODE_STRING 0
#define PRINT_MODE_DATA 0

int
tic6x_section_print_mixed(bfd_vma addr, struct disassemble_info *info)
{
    bfd_byte str[MAX_STR + 1];
    int printable = 1;
    off_t offset = 0 ;
    int i;

    buffer_read_memory (addr, str, MAX_STR, info);
    str[MAX_STR] = 0;

    for (offset = 0 ; offset < 3 ; offset++) {
	printable = 1;
        for (i = offset ; i < offset + strlen((char *)(str + offset)) ; i++) {
            if ((str[i] < 32 || str[i] > 126) && str[i] != '\n' && str[i] != '\t' && str[i] != '\r') {
                printable = 0;
                break;
	    }
        }
    	if (printable && strlen((char *)(str + offset)) > 1) {
		switch (offset) {
		case 0:
			return tic6x_section_print_string(addr, info);
		case 1:
		case 3:
			return tic6x_section_print_byte(addr, info);
		case 2:
			return tic6x_section_print_short(addr, info);
		}
	}
    }

    return tic6x_section_print_word(addr, info);
}
