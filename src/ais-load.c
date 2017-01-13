#include <stdio.h>
#include <stdlib.h>
#include "ais.h"

#define AIS_FUNC(name, label, ac) { ais_function_config_##name, label, ac },
ais_function ais_function_table[ais_function_max] = {
	#include "ais-function-table.h"
};

void ais_section_load(unsigned int **p, ais_opcode_callback_ftype callback)
{
	ais_opcode_info info;
	info.section_load.vma = *(*p)++;
	info.section_load.size = *(*p)++;
	info.section_load.buffer = *p;
	*p += info.section_load.size / sizeof(**p);
	fprintf(stderr, "// Section load 0x%x[0x%x]\n", info.section_load.vma, info.section_load.size);
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
	fprintf(stderr, "// Section Fill type %d 0x%x[0x%x] = 0x%x", type, address, size, pattern);
}

void ais_boot_table(unsigned int **p, ais_opcode_callback_ftype callback)
{
	unsigned int type = *(*p)++;
	ais_vma address = *(*p)++;
	unsigned int data = *(*p)++;
	unsigned int sleep = *(*p)++;
	fprintf(stderr, "// Section boot table %d *%08x = %0x, sleep %d\n", type, address, data, sleep);
}

void ais_function_execute(unsigned int **p, ais_opcode_callback_ftype callback)
{
	int i;
	unsigned int arg = *(*p)++;
	unsigned int func = arg & 0xFFFF;
	unsigned ac = (arg & 0xFFFF0000) >> 16;
	if (func >= ais_function_max) {
		fprintf(stderr, "illegal function code 0x%x\n", arg);
		exit(EXIT_FAILURE);
	}
	ais_function *fn = &ais_function_table[func];
	if (ac != fn->ac) {
		fprintf(stderr, "Arg count mismatch %d but expected %d\n", func, fn->ac);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "// Function %d : %s(", func, fn->name );
	for (i = 0; i < ac ; i++) {
		unsigned int arg = *(*p)++;
		fprintf(stderr, " 0x%x, ", arg);
	}
	fprintf(stderr, ")\n");
}

void ais_jump_and_close(unsigned int **p, ais_opcode_callback_ftype callback)
{
	ais_vma address = *(*p)++;
	fprintf(stderr, "// Close and jump to 0x%x\n", address);
}


void ais_jump(unsigned int **p, ais_opcode_callback_ftype callback)
{
	ais_vma address = *(*p)++;
	fprintf(stderr, "// Jump to 0x%x\n", address);
}


void ais_validate_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
	unsigned int crc = *(*p)++;
	unsigned int seek = *(*p)++;
	fprintf(stderr, "// Validate CRC 0x%x, seek 0x%x\n", crc, seek);
}

void ais_enable_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
	fprintf(stderr, "// Enable CRC\n");
}


void ais_disable_crc(unsigned int **p, ais_opcode_callback_ftype callback)
{
	fprintf(stderr, "// Enable CRC\n");
}

void ais_sequential_read(unsigned int **p, ais_opcode_callback_ftype callback)
{
	fprintf(stderr, "// Sequential read enable\n");
}

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
		fprintf(stderr, "aisread: not an AIS file! (header @%p = 0x%08x)\n", p, *p);
		return;
	}
	fprintf(stderr, "// AIS magic ID\n");
	while ((void *)p < buffer + size) {
		int i;
	        int found = 0;
		unsigned int opcode = *p++;
		for (i = 0 ; i < 10 ; i++) {
			if (ais_opcode_table[i].opcode == opcode) {
				(ais_opcode_table[i].handler)(&p, callback);
				found = 1;
			}
		}
		if (!found) {
			fprintf(stderr, "airead: invalid opcode 0x%08x\n", *p);
			return;
		}
	}
}


