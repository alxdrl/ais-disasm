#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#define PACKAGE "ais-disasm"
#define PACKAGE_VERSION "0.0.0"

#include <dis-asm.h>

#include "list.h"
#include "ais.h"
#include "ais-region.h"

LIST_HEAD(ais_space);

ais_region *get_region_at(ais_vma addr)
{
	struct list_head *entry = NULL;
	ais_region *region;
	list_for_each(entry, &ais_space) {
		region = list_entry(entry, ais_region, list);
		if (addr >= region->start
				&& addr < region->start + region->size)
			return region;
	}
	return NULL;
}

ais_vma get_prev_insn_address(ais_vma current_addr)
{
	ais_vma addr = current_addr - 4;
	if (tic6x_is_16_bits_insn(addr)) {
		addr += 2;
	}
	return addr;
}

ais_vma get_next_insn_address(ais_vma current_addr)
{
	ais_vma addr = current_addr;
	addr += tic6x_is_32_bits_insn(addr) ? 4 : 2;
	return addr;
}

ais_region *punch_space(ais_vma addr, uint32_t size)
{
	ais_region *container;
	container = get_region_at(addr);
	if (container == NULL)
		return NULL;
	if (container->size <= size)
		return NULL;
        ais_region *punch = malloc(sizeof(ais_region));
	if (!punch)
		return NULL;
	punch->start = addr;
	punch->size = size;
	/* [start..end] yields [start..(addr-1)][addr..end] */
	if (container->start == addr) {
		list_add(punch->list, container->list);
		container->start += size;
		container->size -= size;
	} else if (container->start + container->size == addr + size) {
		list_add_tail(punch->list, container->list);
		container->size -= size;
	/* [start..end] yields [start..(addr-1)][addr..(addr+size-1)][(addr+size)..end] */
	} else {
		list_add_tail(punch->list, container->list);
		ais_region *stale = malloc(sizeof(ais_region));
		if (!stale)
			return NULL;
		stale->start = addr + size;
		stale->size = container->start + container->size - stale->start;
		container->size = addr - container->start;
		list_add(stale->list, stale->list);
	}
	return punch;
}
