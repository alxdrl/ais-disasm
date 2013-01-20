#ifndef __AIS_LOAD_H
#	define __AIS_LOAD_H

#include "ais.h"
#include "ais-load.h"

typedef union {
	ais_section_load_info section_load;
} ais_opcode_info;

typedef void (*ais_opcode_callback_ftype) (ais_opcode_info *);
typedef void (*ais_opcode_handler_ftype) (unsigned int **, ais_opcode_callback_ftype);

typedef struct {
	unsigned int opcode;
	ais_opcode_handler_ftype handler;
} ais_opcode;

void aisread(void *buffer, size_t size, ais_opcode_callback_ftype callback);

#endif
