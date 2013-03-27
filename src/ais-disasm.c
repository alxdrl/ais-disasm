#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/mman.h>

#include "ais.h"
#include "ais-print.h"
#include "ais-helper.h"
#include "hashtab.h"

static struct config_t {
    int ais_fd;
    FILE *cmd_file;
    FILE *out_file;
	size_t bufsize;
	void *buffer;
} config_data;

hashtab_t *s_table;

int
buffer_copy_memory (bfd_vma src_vma,
                    bfd_vma dst_vma,
                    unsigned int length,
                    struct disassemble_info *src_info,
                    struct disassemble_info *dst_info)
{
  unsigned int opb = src_info->octets_per_byte;

  if (opb == 0 || dst_info->octets_per_byte != opb)
        /* invalid opb values */
        return EINVAL;

  unsigned int end_addr_offset = length / opb;

  unsigned int src_offset = (src_vma - src_info->buffer_vma) * opb;
  unsigned int src_max_addr_offset = src_info->buffer_length / opb;
  if (src_vma < src_info->buffer_vma
      || src_vma - src_info->buffer_vma > src_max_addr_offset
      || src_vma - src_info->buffer_vma + end_addr_offset > src_max_addr_offset)
    /* Out of bounds.  Use EIO because GDB uses it.  */
    return EIO;

  unsigned int dst_offset = (dst_vma - dst_info->buffer_vma) * opb;
  unsigned int dst_max_addr_offset = dst_info->buffer_length / opb;
  if (dst_vma < dst_info->buffer_vma
      || dst_vma - dst_info->buffer_vma > dst_max_addr_offset
      || dst_vma - dst_info->buffer_vma + end_addr_offset > dst_max_addr_offset)
    /* Out of bounds.  Use EIO because GDB uses it.  */
    return EIO;

  memcpy (dst_info->buffer + dst_offset, src_info->buffer + src_offset, length);

  return 0;
}

static FILE *
do_open(const char *filename, const char *mode)
{
	FILE *f = fopen(filename, mode);
	if (f == NULL) {
		perror("unable to open specified file");
		exit(EXIT_FAILURE);
	}
	return f;
}

static void
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

       case ':':
            fprintf(stderr, "option -%c requires an argument\n", optopt);
			break;

       case '?':
            fprintf(stderr, "unrecognized option: -%c\n", optopt);
			break;

       default:
            fprintf(stderr, "getopt returned character code 0x%x\n", (unsigned int)c);
			exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        fprintf(stderr, "non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
    if (config_data.ais_fd < 0) {
	fprintf(stderr, "no ais file specified\n");
	exit(EXIT_FAILURE);
    }
}

static off_t
filesize(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("unable to get file size");
		exit(errno);
	}
	return st.st_size;
}

static struct disassemble_info tic6x_space_at_0x11800000;
static struct disassemble_info tic6x_space_at_0x11f00000;
static struct disassemble_info tic6x_space_at_0xc0000000;

static void
tic6x_init_section(struct disassemble_info *pinfo, void *buffer, bfd_vma vma, unsigned int size)

{
	init_disassemble_info (pinfo, NULL, (fprintf_ftype)disasm_sprintf);
	pinfo->mach = bfd_arch_tic6x;
	pinfo->endian = BFD_ENDIAN_LITTLE;
	pinfo->buffer = buffer;
	pinfo->buffer_vma = vma;
	pinfo->buffer_length = size;
	pinfo->print_address_func = tic6x_print_address;
}

static struct disassemble_info *
tic6x_get_di(ais_vma vma)
{
	if (vma >= 0x11800000u && vma <= 0x1183ffffu)
		return &tic6x_space_at_0x11800000;
	if (vma >= 0x11f00000u && vma <= 0x11f07fffu)
		return &tic6x_space_at_0x11f00000;
	if (vma >= 0xc0000000u && vma <= 0xcfffffffu)
		return &tic6x_space_at_0xc0000000;
	return 0;
}

static void
tic6x_section_load_callback(ais_opcode_info *info)
{
	ais_vma vma = info->section_load.vma;
	struct disassemble_info *di = tic6x_get_di(vma);
	uint32_t offset = vma - di->buffer_vma;
	void *dst = di->buffer + offset;
	void *src = info->section_load.buffer;
	memcpy(dst, src, info->section_load.size);
}

typedef struct {
	uint32_t val;
	unsigned int lsb:1;
	unsigned int msb:1;
} tic6x_reg_t;

static tic6x_reg_t regs[2][32];

static int
gather_xref(char *s, char *xref)
{
	intmax_t val;
	int regnum;
	int idx;
	char regfile[2];
	xref[0] = 0;
	if (sscanf(s, "%*[][ab012|m !]vk .%*[LSD]%*[12] %jd,%[ab]%d", &val, regfile, &regnum) == 3) {
		idx = regfile[0] - 'a';
		regs[idx][regnum].val = (int32_t)val;
	//	sprintf(xref, "%s%d = 0x%x", regfile, regnum, regs[idx][regnum].val);
	//	return 1;
	} else if (sscanf(s, "%*[][ab012|m !]vkh .%*[LSD]%*[12] %jd,%[ab]%d", &val, regfile, &regnum) == 3) {
		idx = regfile[0] - 'a';
		regs[idx][regnum].val = (regs[idx][regnum].val & 0x0000ffff) | (uint32_t)val;
		char *symbol = tic6x_get_symbol(regs[idx][regnum].val);
		if (symbol) {
			sprintf(xref, "%s%d = %s [xref]", regfile, regnum, symbol);
		} else {
			sprintf(xref, "%s%d = 0x%08x", regfile, regnum, regs[idx][regnum].val);
		}
		return 1;
	}
	return 0;
}

#define XSTR(s) STR(s)
#define STR(s) #s
#define FMTNS(s) "%" XSTR(s##_MAX) "s"
#define FMTNSPR(s) "%-" XSTR(s##_MAX) "s"
#define DATA_MAX 8
#define LABEL_MAX 20
#define MIN(a,b) ((a)<(b)?(a):(b))
static void
tic6x_print_region(ais_vma vma, size_t section_size, tic6x_print_region_ftype tic6x_print_func)
{
	char label[LABEL_MAX + 1];
	bfd_byte data[DATA_MAX];
	static SFILE sfile = {NULL, 0, 0};
	struct disassemble_info *pinfo = tic6x_get_di(vma);
	if (pinfo == 0) {
		fprintf(stderr, "*** error: unable to get disassemble_info for vma 0x%08x\n", vma);
		return;
	}

	ais_vma vmamax = pinfo->buffer_vma + pinfo->buffer_length;
	ais_vma vmaend = vma + section_size;
	if (vmaend > vmamax)
		return;
	alloc_buffer(&sfile);
	pinfo->stream = &sfile;
	while (vma < vmaend && vma >= pinfo->buffer_vma) {
		int i;
		int bytes_used;
		char xref[64];
		sfile.pos = 0;
		bytes_used = (tic6x_print_func)(vma, pinfo);
		if (bytes_used <= 0) {
			fprintf(stderr, "*** error: read %d bytes, broke down at 0x%08x\n", bytes_used, vma);
			break;
		}
		printf("%08x ", vma);
		buffer_read_memory(vma, data, MIN(bytes_used, DATA_MAX) , pinfo);
		for (i = 0 ; i < DATA_MAX ; i++) {
			if (i < bytes_used)
				printf("%02x", data[i]);
			else
				printf("  ");
		}
		tic6x_print_label(vma, label);
		printf(" "FMTNSPR(LABEL) "%s", label , sfile.buffer);
		if (gather_xref(sfile.buffer, xref))
			printf("\t\t\t; %s", xref);
		fputc('\n', stdout);
		vma += bytes_used;
	}
}

static void
zoom_copy_init_table(ais_vma vma)
{
	uint32_t size;
	struct disassemble_info *di = tic6x_get_di(vma);

	fprintf(stderr, "processing Zoom copy table @0x%08x\n", vma);

	buffer_read_memory(vma, (bfd_byte *)&size, sizeof(uint32_t), di);
	while (size != 0) {
		uint32_t dst_vma = 0;
		struct disassemble_info *dst_di = NULL;
		buffer_read_memory(vma + 4, (bfd_byte *)&dst_vma, sizeof(uint32_t), di);
		dst_di = tic6x_get_di(dst_vma);
		if (dst_di == 0) {
			fprintf(stderr, "*** error: entry @%08x unable to get disassembe_info for vma %08x\n", vma, dst_vma);
			continue;
		}
		fprintf(stderr, "processing copy table entry @0x%08x : copy 0x%x bytes from 0x%08x to 0x%08x\n", vma, size, vma + 8, dst_vma);
		buffer_copy_memory(vma + 8, dst_vma, size, di, dst_di);
		vma = (vma + size + 0xf) & 0xfffffff8;
		buffer_read_memory(vma, (bfd_byte *)&size, sizeof(uint32_t), di);
	}
}

#define SYMBOL_MAX 64
#define TOKEN_MAX 10
static void
do_dump()
{
	int nl = 0;
	void *tic6x_mem_0x11800000, *tic6x_mem_0x11f00000, *tic6x_mem_0xc0000000;
	ais_vma addr;
	char line[LINE_MAX];
	char symbol[SYMBOL_MAX + 1];
	char token[TOKEN_MAX + 1];

	tic6x_mem_0x11800000 = mmap(0, 0x00040000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	if (tic6x_mem_0x11800000 == MAP_FAILED) {
		perror("Error mmapping dsp adress space");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "tic6x space 0x11800000 mapped @ %p\n", tic6x_mem_0x11800000);

	tic6x_mem_0x11f00000 = mmap(0, 0x00008000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	if (tic6x_mem_0x11f00000 == MAP_FAILED) {
		perror("Error mmapping dsp adress space");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "tic6x space 0x11800000 mapped @ %p\n", tic6x_mem_0x11800000);

	tic6x_mem_0xc0000000 = mmap(0, 0x00200000, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	if (tic6x_mem_0xc0000000 == MAP_FAILED) {
		perror("Error mmapping dsp adress space");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "tic6x space 0xc0000000 mapped @ %p\n", tic6x_mem_0xc0000000);

	tic6x_init_section (&tic6x_space_at_0x11800000, tic6x_mem_0x11800000, 0x11800000, 0x00040000);
	tic6x_init_section (&tic6x_space_at_0x11f00000, tic6x_mem_0x11f00000, 0x11f00000, 0x00008000);
	tic6x_init_section (&tic6x_space_at_0xc0000000, tic6x_mem_0xc0000000, 0xc0000000, 0x00200000);

	aisread(config_data.buffer, config_data.bufsize, tic6x_section_load_callback);
	zoom_copy_init_table(0xc00c9470);
	while (fgets(line, LINE_MAX, config_data.cmd_file)) {
		int n = 0;
		nl++;
		n = sscanf(line, "%" XSTR(TOKEN_MAX) "s ", token);
		if (n == 0 || n == EOF)
			continue;
		if (strcmp("print", token) == 0) {
			n = sscanf(line, "print " FMTNS(TOKEN), token);
			if (n == 0 || n == EOF) {
				fprintf(stderr, "line %d: invalid print command\n\t\t>>>>> %s\n", nl, line);
				continue;
			}
			if (strcmp("region", token) == 0) {
				uintmax_t m = 0;
				size_t len = 0;
				tic6x_print_region_ftype print_func = NULL;
				n = sscanf(line, "print region %ji,%zi as " FMTNS(TOKEN), &m, &len, token);
				if (n < 2 || n == EOF) {
					fprintf(stderr, "line %d: invalid region specification (%d) \n\t\t>>>>> %s\n", nl, n, line);
					continue;
				}
				addr = m;
				print_func = print_insn_tic6x;
				if (n == 3) {
					if (strcmp("string", token) == 0) {
						print_func = tic6x_section_print_string;
					} else if (strcmp("word", token) == 0) {
						print_func = tic6x_section_print_word;
					} else if (strcmp("code", token) == 0) {
						print_func = print_insn_tic6x;
					} else {
						fprintf(stderr, "line %d: unknown format \'%s\'\n\t\t>>>>> %s\n", nl, token, line);
						continue;
					}
				}
				tic6x_print_region(addr, len, print_func);
			}
		} else if (strcmp("define", token) == 0) {
			uintmax_t m = 0;
			n = sscanf(line, "define %ji " FMTNS(SYMBOL), &m, symbol);
			if (n < 2 || n == EOF) {
				fprintf(stderr, "line %d: invalid define command\n\t\t>>>>> %s\n", nl, line);
				continue;
			}
			addr = m;
			size_t slen = strlen(symbol);
			void *ret = ht_insert(s_table, &addr, sizeof(addr), symbol, slen + 1);
			if (ret == NULL) {
				perror("unable to insert symbol into table");
				exit(EXIT_FAILURE);
			}
		} else {
			fprintf(stderr, "line %d: unknown command %s\n\t\t>>>>> %s\n", nl, token, line);
			continue;
		}
	}
}

void
do_init_sym_table()
{
#define HASH_INIT_SIZE 1024
	s_table = ht_init(HASH_INIT_SIZE, NULL);
	if (s_table == NULL) {
		fprintf(stderr, "unable to initialize symbol table\n");
		exit(EXIT_FAILURE);
	}
}

void
do_init()
{
	do_init_sym_table();
}

int
main(int argc, char **argv)
{
	do_init();
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
