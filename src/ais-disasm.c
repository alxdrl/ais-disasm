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
#include "libiberty.h"

typedef uint32_t ais_vma;

typedef enum {
	ais_function_config_pll,
	ais_function_config_clock,
        ais_function_config_emifb_sd,
	ais_function_config_emifa_sd,
	ais_function_config_emifa_ce,
	ais_function_config_pll_clock,
	ais_function_config_psc,
	ais_function_config_pinmux,
	ais_fonction_max,
} ais_function_id;

typedef struct {
	ais_function_id id;
	const char *name;
	unsigned int ac;
} ais_function;

typedef struct {
	ais_vma vma;
	void *buffer;
	unsigned int size;
} ais_section_load_info;

typedef union {
	ais_section_load_info section_load;
} ais_opcode_info;

typedef void (*ais_opcode_callback_ftype) (ais_opcode_info *);
typedef void (*ais_opcode_handler_ftype) (unsigned int **, ais_opcode_callback_ftype);
typedef struct {
	unsigned int opcode;
	ais_opcode_handler_ftype handler;
} ais_opcode;

#define AIS_FUNC(name, label, ac) { ais_function_config_##name, label, ac },
ais_function ais_function_table[ais_fonction_max] = {
	AIS_FUNC(pll, "PLL0 Configuration", 2)
        AIS_FUNC(clock, "Peripheral Clock Configuration", 1)
        AIS_FUNC(emifb_sd, "EMIFB SDRAM Configuration", 4)
        AIS_FUNC(emifa_sd, "EMIFA SDRAM Configuration", 4)
        AIS_FUNC(emifa_ce, "EMIFA CE Space Configuration", 4)
        AIS_FUNC(pll_clock, "PLL and Clock Configuration", 3)
        AIS_FUNC(psc, "Power and Sleep Controller Configuration", 1)
        AIS_FUNC(pinmux, "Pinmux Configuration", 3)
};

void ais_section_load(unsigned int **p, ais_opcode_callback_ftype callback)
{
    ais_opcode_info info;
    info.section_load.vma = *(*p)++;
    info.section_load.size = *(*p)++;
    info.section_load.buffer = *p;
    *p += info.section_load.size / sizeof(**p);
    printf("// Section load 0x%x[0x%x]\n", info.section_load.vma, info.section_load.size);
    if (callback) {
	(callback)(&info);
    }
}

void ais_section_fill(unsigned int **p, ais_opcode_callback_ftype callback)
{
    ais_vma address = *(*p)++;
    unsigned int size = *(*p)++;
    unsigned int type = *(*p)++;
    unsigned int pattern = *(*p)++;
    printf("// Section Fill type %d 0x%x[0x%x] = 0x%x", type, address, size, pattern);
}

void ais_boot_table(unsigned int **p, ais_opcode_callback_ftype callback)
{
    unsigned int type = *(*p)++;
    ais_vma address = *(*p)++;
    unsigned int data = *(*p)++;
    unsigned int sleep = *(*p)++;
    printf("section boot table %d *%08x = %0x, sleep %d\n", type, address, data, sleep);
}

void ais_function_execute(unsigned int **p, ais_opcode_callback_ftype callback)
{
    int i;
    unsigned int arg = *(*p)++;
    unsigned int func = arg & 0xFFFF;
    unsigned ac = (arg & 0xFFFF0000) >> 16;
    if (func >= ais_fonction_max) {
	printf("illegal function code 0x%x\n", arg);
	exit(EXIT_FAILURE);
    }
    ais_function *fn = &ais_function_table[func];
    if (ac != fn->ac) {
	printf("Arg count mismatch %d but expected %d\n", func, fn->ac, ac);
	exit(EXIT_FAILURE);
    }
    printf("// Function %d : %s(", func, fn->name );
    for (i = 0; i < ac ; i++) {
	unsigned int arg = *(*p)++;
	printf(" 0x%x, ", arg);
    }
    printf(")\n");
}

void ais_jump_and_close(unsigned int **p, ais_opcode_callback_ftype callback)
{
    ais_vma address = *(*p)++;
    printf("// Close and jump to 0x%x\n", address);
}


void ais_jump(unsigned int **p, ais_opcode_callback_ftype callback)
{
    ais_vma address = *(*p)++;
    printf("// Jump to 0x%x\n", address);
}


void ais_validate_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
    unsigned int crc = *(*p)++;
    unsigned int seek = *(*p)++;
    printf("// Validate CRC 0x%x, seek 0x%x\n", crc, seek);
}

void ais_enable_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
    printf("// Enable CRC\n");
}


void ais_disable_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
    printf("// Enable CRC\n");
}

void ais_sequential_read(unsigned int **p, ais_opcode_callback_ftype callback)
{
    printf("// Sequential read enable\n");
}

#define AIS_MAGIC                   0x41504954
#define AIS_OPCODE_SECTION_LOAD     0x58535901
#define AIS_OPCODE_VALIDATE_CRC     0x58535902
#define AIS_OPCODE_ENABLE_CRC       0x58535903
#define AIS_OPCODE_DISABLE_CRC      0x58535904
#define AIS_OPCODE_JUMP             0x58535905
#define AIS_OPCODE_JUMP_AND_CLOSE   0x58535906
#define AIS_OPCODE_BOOT_TABLE       0x58535907
#define AIS_OPCODE_SECTION_FILL     0x5853590a
#define AIS_OPCODE_FUNCTION_EXECUTE 0x5853590d
#define AIS_OPCODE_SEQUENTIAL_READ  0x58535963

#define AIS_OPCODE(a, b) { AIS_OPCODE_##a, ais_##b },
ais_opcode ais_opcode_table[10] = {
	AIS_OPCODE(SECTION_LOAD, section_load)
	AIS_OPCODE(VALIDATE_CRC, validate_crc)
	AIS_OPCODE(ENABLE_CRC, enable_crc)
	AIS_OPCODE(DISABLE_CRC, disable_crc)
	AIS_OPCODE(JUMP, jump)
	AIS_OPCODE(JUMP_AND_CLOSE, jump_and_close)
	AIS_OPCODE(BOOT_TABLE, boot_table)
	AIS_OPCODE(SECTION_FILL, section_fill)
	AIS_OPCODE(FUNCTION_EXECUTE, function_execute)
	AIS_OPCODE(SEQUENTIAL_READ, sequential_read)
};

void aisread(void *buffer, size_t size, ais_opcode_callback_ftype callback)
{
	unsigned int *p = buffer;
	if (*p++ != AIS_MAGIC) {
		printf("ais: not an AIS file! (header @%p = 0x%08x)\n", p, *p);
		return;
	}
	printf("// AIS magic ID\n");
	while ((void *)p < buffer + size) {
                int i;
		unsigned int opcode = *p++;
                bfd_boolean found = FALSE; 
		for (i = 0 ; i < 10 ; i++) {
			if (ais_opcode_table[i].opcode == opcode) {
				(ais_opcode_table[i].handler)(&p, callback);
				found = TRUE;
			}
		}
		
		if (!found) {
			printf("ais: invalid opcode 0x%08x\n", *p);
			return;
		}
	}
}

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
    pinfo->buffer_length = size ;
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
    printf("vma = %08X, vmamax = %08X, vmaend = %08X size = %x\n", vma, vmamax, vmaend, (unsigned int)section_size);
    if (vmaend > vmamax)
	return;
    if (sfile.buffer == NULL) {
    	sfile.alloc = 120;
 	sfile.buffer = (void *)xmalloc (sfile.alloc);
    }
    pinfo->stream = &sfile;
    printf(";\n; section @0x%08x\n;\n", (unsigned int)vma);
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
	        snprintf(format, 31, "%%08x %%0%1dx%s%%s\n", bytes_used * 2, &("         "[bytes_used * 2])); 
        	printf(format, vma, word, sfile.buffer);
        } else {
		printf("%08x          %s\n", (unsigned int)vma, (char *)sfile.buffer);
        }
        vma += bytes_used;
    }
}

int main(int argc, char **argv)
{
    int fd;
    int length;
    void *buffer;
    size_t size;
    off_t offset;
    bfd_vma org;
    bfd_vma vma;

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
    printf("file mapped @ %p\n", buffer);

    void *tic6x_mem_0x11800000 = mmap(0, 0x00200000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (tic6x_mem_0x11800000 == MAP_FAILED) {
	perror("Error mmapping dsp adress space");
	exit(EXIT_FAILURE);
    }
    printf("tic6x space 0x11800000 mapped @ %p\n", tic6x_mem_0x11800000);

    void *tic6x_mem_0xc0000000 = mmap(0, 0x00200000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (tic6x_mem_0xc0000000 == MAP_FAILED) {
	perror("Error mmapping dsp adress space");
	exit(EXIT_FAILURE);
    }
    printf("tic6x space 0xc0000000 mapped @ %p\n", tic6x_mem_0xc0000000);


    init_disassemble_info (&tic6x_space_at_0x11800000, NULL, (fprintf_ftype)disasm_sprintf);
    init_disassemble_info (&tic6x_space_at_0xc0000000, NULL, (fprintf_ftype)disasm_sprintf);
    tic6x_init_section (&tic6x_space_at_0x11800000, tic6x_mem_0x11800000, 0x11800000, 0x00200000);
    tic6x_init_section (&tic6x_space_at_0xc0000000, tic6x_mem_0xc0000000, 0xc0000000, 0x00200000);

    aisread(buffer, size, tic6x_section_load_callback);

    tic6x_print_region(0x1180b000,  0xeac0, (tic6x_print_region_ftype)print_insn_tic6x);
    tic6x_print_region(0xc0000000, 0x64e20, (tic6x_print_region_ftype)print_insn_tic6x);
    tic6x_print_region(0xc0064e20,  0x4570, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc0069390,  0x16e0, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc006aa70, 0x23174, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc008dbe4,  0x012c, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc008dd20,  0x12b8, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc008efd8,  0x0130, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc008f108,  0x038C, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc008f494,  0x0121, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc008f5b4,  0x3a74, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc0093028,  0x00ea, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0093e04,  0x00c6, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0095ac0,  0x00b0, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0096208,  0x0098, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc00974f0,  0x0074, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0097bf0,  0x0056, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0098010,  0x0056, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0098274,  0x0050, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc0098e0d,  0x002c, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc009968c,  0x0027, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc009a3c0,  0x0047, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc009a560,  0x0010, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc009a5b0,  0x0018, (tic6x_print_region_ftype)tic6x_section_print_string);
    tic6x_print_region(0xc009a5c8,  0x0144, (tic6x_print_region_ftype)tic6x_section_print_word);
    tic6x_print_region(0xc009a70c,  0x0031, (tic6x_print_region_ftype)tic6x_section_print_string);
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

