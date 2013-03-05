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
#include <getopt.h>
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

static struct config_t {
    int ais_fd;
    FILE *cmd_file;
    FILE *out_file;
	size_t bufsize;
	void *buffer;
} config_data;

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

FILE *
do_open(const char *filename, const char *mode)
{
	FILE *f = fopen(filename, mode);
	if (f == NULL) {
		perror("unable to open specified file");
		exit(EXIT_FAILURE);
	}
	return f;
}

void
do_config(int argc, char **argv)
{
   int c;

   config_data.ais_fd = -1;
   config_data.cmd_file = stdin;
   config_data.out_file = stdout;

   while (1) {
       int option_index = 0;
       static struct option long_options[] = {
           {"ais-data", required_argument, 0,  'a' },
           {"input",    required_argument, 0,  'i' },
           {"output",   required_argument, 0,  'o' },
           {0,          0,                 0,  0 }
       };

       c = getopt_long(argc, argv, "a:i:o:",
                 long_options, &option_index);
       if (c == -1)
            break;

       switch (c) {
       case 'a':
            fprintf(stderr, "option a with value '%s'\n", optarg);
			config_data.ais_fd = open(optarg, O_RDONLY);
			if (config_data.ais_fd == -1) {
				perror("unable to open file for reading");
				exit(EXIT_FAILURE);
			}
            break;

       case 'i':
            fprintf(stderr, "option i with value '%s'\n", optarg);
			config_data.out_file = do_open(optarg, "r");
            break;

       case 'o':
            fprintf(stderr, "option o with value '%s'\n", optarg);
			config_data.out_file = do_open(optarg, "w");
            break;

       default:
            fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
        }
    }

   if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
}

void
tic6x_print_address (bfd_vma addr, struct disassemble_info *info)
{
  char buf[30];

  tic6x_sprintf_vma (buf, addr);
  (*info->fprintf_func) (info->stream, "0x%s", buf);
}

size_t
filesize(int fd)
{
	struct stat st;
	fprintf(stderr, "getting file size on descriptor %d", fd);
	if (fstat(fd, &st) < 0) {
		perror("unable to get file size");
		exit(errno);
	}
	return st.st_size;
}

typedef int (*tic6x_print_region_ftype) (bfd_vma addr, struct disassemble_info *info);

struct disassemble_info tic6x_space_at_0x11800000;
struct disassemble_info tic6x_space_at_0xc0000000;

void
tic6x_init_section(struct disassemble_info *pinfo, void *buffer, bfd_vma vma, size_t size)

{
	pinfo->mach = bfd_arch_tic6x;
	pinfo->endian = BFD_ENDIAN_LITTLE;
	pinfo->buffer = buffer;
	pinfo->buffer_vma = vma;
	pinfo->buffer_length = size;
	pinfo->print_address_func = tic6x_print_address;
}

struct disassemble_info *
tic6x_get_di(ais_vma vma)
{
	if (vma >= 0x11800000 && vma <= 0xbfffffff)
		return &tic6x_space_at_0x11800000;
	if (vma >= 0xc0000000 && vma <= 0xffffffff)
		return &tic6x_space_at_0xc0000000;
	return 0;
}

void
tic6x_section_load_callback(ais_opcode_info *info)
{
	ais_vma vma = info->section_load.vma;
	struct disassemble_info *di = tic6x_get_di(vma);
	uint32_t offset = vma - di->buffer_vma;
	void *dst = di->buffer + offset;
	void *src = info->section_load.buffer;
	memcpy(dst, src, info->section_load.size);
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

void
tic6x_print_region(ais_vma vma, size_t section_size, tic6x_print_region_ftype tic6x_print)
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

void
do_dump()
{
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

	char line[LINE_MAX];
	int nl = 0;

	aisread(config_data.buffer, config_data.bufsize, tic6x_section_load_callback);

	while (fgets(line, LINE_MAX, config_data.cmd_file)) {
		char *command = NULL;
		int n = 0;
		nl++;
		n = sscanf(line, "%as ", &command);
		if (n == 0 || n == EOF)
			continue;
		if (strcmp("print", command) == 0) {
			char *what = NULL;
			n = sscanf(line, "print %as", &what);
			if (n == 0 || n == EOF) {
				fprintf(stderr, "line %d: invalid print command\n\t\t>>>>> %s\n", nl, line);
				continue;
			}
			if (strcmp("region", what) == 0) {
				uintmax_t addr = 0;
				size_t len = 0;
				char *format = NULL;
				tic6x_print_region_ftype print_func = NULL;
				n = sscanf(line, "print region %ji,%zi as %as", &addr, &len, &format);
				fprintf(stderr, "addr = 0x%ju, len = 0x%zu\n", addr, len);
				if (n < 2 || n == EOF) {
					fprintf(stderr, "line %d: invalid region specification (%d) \n\t\t>>>>> %s\n", nl, n, line);
					continue;
				}
				print_func = print_insn_tic6x;
				if (n == 3) {
					if (strcmp("string", format) == 0) {
						print_func = tic6x_section_print_string;
					} else if (strcmp("word", format) == 0) {
						print_func = tic6x_section_print_word;
					} else if (strcmp("code", format) == 0) {
						print_func = print_insn_tic6x;
					} else {
						fprintf(stderr, "line %d: unknown format \'%s\'\n\t\t>>>>> %s\n", nl, format, line);
						continue;
					}
				}
				if (format != NULL) 
					free(format);
				tic6x_print_region(addr, len, print_func);
			}
			if (what != NULL)
				free(what);
		} else {
			fprintf(stderr, "line %d: unknown command %s\n\t\t>>>>> %s\n", nl, command, line);
			continue;
		}
		if (command != NULL)
			free(command);
	}
}

int
main(int argc, char **argv)
{
	do_config(argc, argv);

	config_data.bufsize = filesize(config_data.ais_fd);
	config_data.buffer = mmap(0, config_data.bufsize, PROT_READ, MAP_SHARED, config_data.ais_fd, 0);

	if (config_data.buffer == MAP_FAILED) {
		close(config_data.ais_fd);
		perror("unable to map file");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "file mapped @ %p\n", config_data.buffer);

	do_dump();

	if (munmap(config_data.buffer, config_data.bufsize) == -1) {
		perror("unable to unmap file");
	}
	close(config_data.ais_fd);
	return EXIT_SUCCESS;
}

