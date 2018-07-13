#include "Heap.h"
#include <atomic>

namespace Nirvana {

using namespace std;

UWord HeapBase::sm_commit_unit;
UWord HeapBase::sm_optimal_commit_unit;
Octet* HeapBase::sm_space_begin;
Octet* HeapBase::sm_space_end;

HeapBase::Partition* Heap32::sm_part_table;

UWord Heap64::sm_table_block_size;
HeapBase::Partition** Heap64::sm_part_table;

Pointer HeapBase::allocate (Directory* part, UWord size, UWord allocation_unit)
{
	UWord units = (size + allocation_unit - 1) / allocation_unit;
	Word unit = part->allocate (units, g_protection_domain_memory);
	if (unit >= 0) {
		Pointer p = (Octet*)(part + 1) + unit * allocation_unit;
		try {
			commit_heap (p, size);
		} catch (...) {
			part->release (unit, unit + units);
			throw;
		}
		return p;
	}
	return 0;
}

HeapBase::Partition& Heap32::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	Partition* p = sm_part_table + ((Octet*)part - sm_space_begin) / MIN_PARTITION_SIZE;
	if (sparse_table (table_bytes ()))
		g_protection_domain_memory->commit (p, sizeof (Partition));
	p->set (part, allocation_unit);
	assert (!p->next);
	return *p;
}

const HeapBase::Partition* Heap32::get_partition (const void* address)
{
	if (!valid_address (address))
		throw BAD_PARAM ();
	const Partition* p = sm_part_table + ((Octet*)address - sm_space_begin) / MIN_PARTITION_SIZE;
	if (
		(!sparse_table (table_bytes ()) || g_protection_domain_memory->is_readable (p, sizeof (Partition)))
	&&
		p->dir_and_unit
	)
		return p;

	return 0;
}

HeapBase::Partition& Heap64::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	UWord idx = ((Octet*)part - sm_space_begin) / MIN_PARTITION_SIZE;
	UWord i0 = idx / sm_table_block_size;
	UWord i1 = idx % sm_table_block_size;
	Partition** pblock = sm_part_table + i0;
	if (sparse_table (table_bytes ()))
		g_protection_domain_memory->commit (pblock, sizeof (Partition*));
	Partition* block = atomic_load ((volatile atomic <Partition*>*)pblock);
	UWord block_size = sm_table_block_size * sizeof (Partition);
	bool commit = false;
	if (!block) {
		commit = true;
		block = (Partition*)g_protection_domain_memory->allocate (0, block_size, Memory::RESERVED | Memory::ZERO_INIT);
		Partition* cur = 0;
		if (!atomic_compare_exchange_strong ((volatile atomic <Partition*>*)pblock, &cur, block)) {
			g_protection_domain_memory->release (block, block_size);
			block = cur;
		}
	} else
		commit = block_size > sm_commit_unit;

	Partition* p = block + i1;
	if (commit)
		g_protection_domain_memory->commit (p, sizeof (Partition));
	p->set (part, allocation_unit);
	return *p;
}

const HeapBase::Partition* Heap64::get_partition (const void* address)
{
	if (!valid_address (address))
		throw BAD_PARAM ();

	UWord idx = ((Octet*)address - sm_space_begin) / MIN_PARTITION_SIZE;
	UWord i0 = idx / sm_table_block_size;
	UWord i1 = idx % sm_table_block_size;
	Partition* const* pblock = sm_part_table + i0;
	if (!sparse_table (table_bytes ()) || g_protection_domain_memory->is_readable (pblock, sizeof (Partition*))) {
		const Partition* p = *pblock;
		if (p) {
			p += i1;
			if (
				(sm_table_block_size * sizeof (Partition) <= sm_commit_unit || g_protection_domain_memory->is_readable (p, sizeof (Partition)))
				&&
				p->dir_and_unit
			)
				return p;
		}
	}

	return 0;
}

Pointer Heap::allocate (UWord size)
{
	Partition* last_part;
	Pointer p = HeapBase::allocate (size, last_part);
	if (!p) {
		Directory* dir = create_partition (m_allocation_unit);
		Partition* new_part = 0;
		try {
			new_part = &add_partition (dir, m_allocation_unit);
			for (;;) {
				Partition* next_part = 0;
				if (atomic_compare_exchange_strong ((volatile atomic <Partition*>*)&last_part->next, &next_part, new_part))
					next_part = new_part;
				if (p = HeapBase::allocate (*next_part, size)) {
					if (next_part != new_part)
						new_part->release ();
					break;
				} else
					last_part = next_part;
			}
		} catch (...) {
			if (new_part)
				new_part->release ();
			else
				g_protection_domain_memory->release (dir, partition_size (m_allocation_unit));
			throw;
		}
	}

	return p;
}

}
