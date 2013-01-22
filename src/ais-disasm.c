#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PACKAGE "ais-disasm"
#define PACKAGE_VERSION "0.0.0"

#include "dis-asm.h"
#include "libiberty/libiberty.h"

#include "ais.h"
#include "ais-load.h"

#define TIC6X_VMA_FMT "l"
#define tic6x_sprintf_vma(s,x) sprintf (s, "%08" TIC6X_VMA_FMT "x", x)

/* Pseudo FILE object for strings.  */
typedef struct
{
  char *buffer;
  size_t pos;
  size_t alloc;
} SFILE;

/* sprintf to a "stream".  */
static int ATTRIBUTE_PRINTF_2
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

void
tic6x_print_address (bfd_vma addr, struct disassemble_info *info)
{
  char buf[30];

  tic6x_sprintf_vma (buf, addr);
  (*info->fprintf_func) (info->stream, "0x%s", buf);
}


size_t filesize(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("unable to get file size");
		exit(errno);
	}
	return st.st_size;
}

typedef int (*tic6x_print_region_ftype) (bfd_vma addr, struct disassemble_info *info);

struct disassemble_info tic6x_space_at_0x11800000;
struct disassemble_info tic6x_space_at_0xc0000000;

void tic6x_init_section(struct disassemble_info *pinfo, void *buffer, bfd_vma vma, size_t size)

{
	pinfo->mach = bfd_arch_tic6x;
	pinfo->endian = BFD_ENDIAN_LITTLE;
	pinfo->buffer = buffer;
	pinfo->buffer_vma = vma;
	pinfo->buffer_length = size;
	pinfo->print_address_func = tic6x_print_address;
}

struct disassemble_info *tic6x_get_di(ais_vma vma)
{
	if (vma >= 0x11800000 && vma <= 0xbfffffff)
		return &tic6x_space_at_0x11800000;
	if (vma >= 0xc0000000 && vma <= 0xffffffff)
		return &tic6x_space_at_0xc0000000;
	return 0;
}

void tic6x_section_load_callback(ais_opcode_info *info)
{
	ais_vma vma = info->section_load.vma;
	struct disassemble_info *di = tic6x_get_di(vma);
	uint32_t offset = vma - di->buffer_vma;
	void *dst = di->buffer + offset;
	void *src = info->section_load.buffer;
	memcpy(dst, src, info->section_load.size);
}

int tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info)
{
     unsigned int word;
     buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
     info->fprintf_func(info->stream, ".word 0x%08x", word);
     return 4;
}

int tic6x_section_print_string(bfd_vma addr, struct disassemble_info *info)
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

void tic6x_print_region(ais_vma vma, size_t section_size, tic6x_print_region_ftype tic6x_print)
{
	static SFILE sfile = {NULL, 0, 0};
	struct disassemble_info *pinfo = tic6x_get_di(vma);
	ais_vma vmamax = pinfo->buffer_vma + pinfo->buffer_length;
	ais_vma vmaend = vma + section_size;
	if (vmaend > vmamax)
	return;
	if (sfile.buffer == NULL) {
		sfile.alloc = 120;
		sfile.buffer = (void *)xmalloc (sfile.alloc);
	}
	pinfo->stream = &sfile;
	printf(";\n; section @0x%08x, size = %x\n;\n", (unsigned int)vma, (unsigned int)section_size);
	while (vma < vmaend && vma >= pinfo->buffer_vma) {
        unsigned int bytes_used;
        char format[32];
        unsigned int word = 0;
        sfile.pos = 0;
        bytes_used = tic6x_print(vma, pinfo);
		if (bytes_used <= 0) {
        	printf("read %d bytes, broke down at 0x%08x\n", bytes_used, vma);
			break;
		}
        if (bytes_used <= 4) {
	        buffer_read_memory (vma, (bfd_byte *)&word, bytes_used, pinfo);
	        snprintf(format, 32, "0x%%08x %%0%1dx%s%%s%%s\n", bytes_used * 2, &("         "[bytes_used * 2])); 
        	printf(format, vma, word, "                    " , sfile.buffer);
        } else {
			printf("0x%08x          %s%s\n", (unsigned int)vma, "                    ", (char *)sfile.buffer);
	    }
        vma += bytes_used;
	}
}

int main(int argc, char **argv)
{
	int fd;
	void *buffer;
	size_t size;

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	size = filesize(fd);

	buffer = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
	if (buffer == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "file mapped @ %p\n", buffer);

	void *tic6x_mem_0x11800000 = mmap(0, 0x00200000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	if (tic6x_mem_0x11800000 == MAP_FAILED) {
		perror("Error mmapping dsp adress space");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "tic6x space 0x11800000 mapped @ %p\n", tic6x_mem_0x11800000);

	void *tic6x_mem_0xc0000000 = mmap(0, 0x00200000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	if (tic6x_mem_0xc0000000 == MAP_FAILED) {
		perror("Error mmapping dsp adress space");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "tic6x space 0xc0000000 mapped @ %p\n", tic6x_mem_0xc0000000);

	init_disassemble_info (&tic6x_space_at_0x11800000, NULL, (fprintf_ftype)disasm_sprintf);
	init_disassemble_info (&tic6x_space_at_0xc0000000, NULL, (fprintf_ftype)disasm_sprintf);
	tic6x_init_section (&tic6x_space_at_0x11800000, tic6x_mem_0x11800000, 0x11800000, 0x00200000);
	tic6x_init_section (&tic6x_space_at_0xc0000000, tic6x_mem_0xc0000000, 0xc0000000, 0x00200000);

	aisread(buffer, size, tic6x_section_load_callback);

	tic6x_print_region(0x1180b000,  0xeac0, (tic6x_print_region_ftype)print_insn_tic6x);
	tic6x_print_region(0xc0000000, 0x64e20, (tic6x_print_region_ftype)print_insn_tic6x);
	tic6x_print_region(0xc0064e20,  0x4570, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0069390,  0x16e0, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc006aa70,  0xc588, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0076ff8,  0x0390, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0077388,  0x0408, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0077790, 0x16454, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc008dbe4,  0x013c, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc008dd20,  0x12b8, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc008efd8,  0x0130, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc008f108,  0x038C, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc008f494,  0x0124, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc008f5b8,  0x3a70, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0093028,  0x00ec, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0093114,  0x0cf0, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0093e04,  0x00c6, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0093eca,  0x1bf6, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0095ac0,  0x00b0, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0095b70,  0x0698, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0096208,  0x0098, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc00962a0,  0x1250, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc00974f0,  0x0074, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0097564,  0x068c, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0097bf0,  0x0058, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0097c48,  0x03c8, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0098010,  0x0058, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0098068,  0x020c, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0098274,  0x0050, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc00982c4,  0x0b48, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0098e0c,  0x0030, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0098e3c,  0x0850, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009968c,  0x002c, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc00996b8,  0x050c, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0099bc4,  0x0024, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc0099be8,  0x0574, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a15c,  0x0018, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a174,  0x0124, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a298,  0x003c, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a2d4,  0x00ec, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a3c0,  0x0048, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a408,  0x0158, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a560,  0x0010, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a570,  0x0040, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a5b0,  0x0018, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a5c8,  0x0144, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc009a70c,  0x0034, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc009a740,  0x013c, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc00c9470,  0xc318, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc00d5788,  0x0068, (tic6x_print_region_ftype)tic6x_section_print_string);
	tic6x_print_region(0xc00d57f0,  0x2a48, (tic6x_print_region_ftype)tic6x_section_print_word);
	tic6x_print_region(0xc0110000, 0x1603c, (tic6x_print_region_ftype)tic6x_section_print_word);

	if (munmap(buffer, size) == -1) {
		perror("Error un-mmapping the file");
	}
	close(fd);
	return EXIT_SUCCESS;
}

