#include <ctype.h>
#include <list.h>

typedef uint32_t ais_vma;

LIST_HEAD(ais_space);

typedef struct {
	struct list_head *list;
	ais_vma start;
	uint32_t size;
	void *data;
} ais_region;

ais_vma get_prev_insn_address()
{
	ais_vma addr = current_addr - 4;
	if (tic6x_is_16_bits_insn(addr))
		addr +=2
		assert(tic6x_is_16_bits_insn(addr));
	else
		assert(tic6x_is_32_bits_insn(addr));
	return addr;
}

ais_vma get_next_insn_address()
{
	ais_vma addr = current_addr;
	addr += tic6x_is_32_bits_insn(addr) ? 4 : 2; 
	return addr;
}

ais_region *punch_space(ais_vma start, uint32_t size)
{
	ais_region *container;
	container = search containing region
	if (container == NULL)
		return 0;
	if (container->size <= size)
		return 0;
        ais_region *punch = malloc(sizeof(ais_region));
	if (!punch)
		return 0;
	punch->start = start;
	punch->size = size;
	if (container->start == start) {
		// two regions
		list_add(punch->list, container->list);
		container->start += size;
		container->size -= size;
	} else if (container->start + container->size == start + size) {
		// two regions
		list_add_tail(punch->list, container->list);
		container->size -= size;
	} else {
		list_add_tail(punch->list, container->list);
		// three regions
		ais_region *stale = malloc(sizeof(ais_region));
		if (!stale)
			return 1;
		// create stale region
		stale->start = start + size;
		stale->size = container->start + container->size - stale->start
		// adjust container size
		container->size = start - container->start;
		list_add(stale->list, stale->list);
	}
	return punch;
}
