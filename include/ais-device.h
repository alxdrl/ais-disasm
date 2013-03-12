#ifndef __AIS_DEVICE_H
#	define __AIS_DEVICE_H

#include <stdint.h>

#define AIS_DEV_MAX_REGION

typedef uint32_t ais_vma;

typedef enum {
	ais_region_rom,
	ais_region_ram,
	ais_region_io,
	ais_region_reserved,
} ais_region_type;

typedef struct {
	char *desc;
	ais_vma start;
	ais_vma end;
	void *backing_store;
} ais_mem_region;

typedef struct {
	int region_count;
	ais_mem_region regions[AIS_DEV_MAX_REGION];
} ais_dev_mem;

typedef struct {
	ais_dev_mem mem;
} ais_dev;

#endif
