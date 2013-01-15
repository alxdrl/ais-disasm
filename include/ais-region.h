#ifndef __AIS_REGION_H
#	define __AIS_REGION_H

#include <stdint.h>
#include "list.h"
#include "ais.h"

typedef struct {
	struct list_head *list;
	ais_vma start;
	size_t size;
	void *data;
} ais_region;

#endif
