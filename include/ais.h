#include <stdint.h>

#ifndef __AIS_H
#	define __AIS_H

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
	ais_function_max,
} ais_function_id;

typedef struct {
	ais_vma vma;
	void *buffer;
	unsigned int size;
} ais_section_load_info;

typedef struct {
	ais_function_id id;
	const char *name;
	unsigned int ac;
} ais_function;

#endif
