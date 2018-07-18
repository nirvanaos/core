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
		assert (unit < Directory::UNIT_COUNT);
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

HeapBase::HeapBase (ULong allocation_unit) :
	m_part_list (0)
{
	if (allocation_unit <= HEAP_UNIT_MIN)
		m_allocation_unit = HEAP_UNIT_MIN;
	else if (allocation_unit >= HEAP_UNIT_MAX)
		m_allocation_unit = HEAP_UNIT_MAX;
	else
		m_allocation_unit = (UWord)1 << (32 - nlz (allocation_unit - 1));
}

bool HeapBase::Partition::allocate (Pointer p, UWord size, Flags flags) const
{
	Directory* dir = directory ();
	Octet* heap = (Octet*)(dir + 1);
	UWord offset = (Octet*)p - heap;
	UWord au = allocation_unit ();
	UWord begin = offset / au;
	UWord end;

	if (flags & Memory::EXACTLY)
		end = (offset + size + au - 1) / au;
	else
		end = begin + (size + au - 1) / au;
	
	if (dir->allocate (begin, end, g_protection_domain_memory)) {
		if (!(flags & Memory::EXACTLY))
			p = heap + begin * au;

		try {
			commit_heap (p, size);
		} catch (...) {
			release (p, size);
			throw;
		}

		if (flags & Memory::ZERO_INIT)
			zero ((Octet*)p, (Octet*)p + size);

		return true;
	}
	return false;
}

HeapBase::Partition& Heap32::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	UWord offset = (Octet*)part - sm_space_begin;
	Partition* begin = sm_part_table + offset / MIN_PARTITION_SIZE;
	Partition* end = sm_part_table + (offset + partition_size (allocation_unit)) / MIN_PARTITION_SIZE;
	if (sparse_table (table_bytes ()))
		g_protection_domain_memory->commit (begin, (end - begin) * sizeof (Partition));
	for (Partition* p = begin; p != end; ++p)
		p->set (part, allocation_unit);
	return *begin;
}

void Heap32::remove_partition (Partition& part)
{
	Partition* begin = &part;
	Partition* end = sm_part_table + ((Octet*)begin->directory () + partition_size (begin->allocation_unit ()) - sm_space_begin) / MIN_PARTITION_SIZE;
	for (Partition* p = begin; p != end; ++p)
		p->clear ();
}

inline const HeapBase::Partition* Heap32::partition (UWord idx)
{
	const Partition* p = sm_part_table + idx;
	if (!sparse_table (table_bytes ()) || g_protection_domain_memory->is_readable (p, sizeof (Partition)))
		return p;
	return 0;
}

HeapBase::Partition& Heap64::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	UWord offset = (Octet*)part - sm_space_begin;
	UWord idx = offset / MIN_PARTITION_SIZE;
	UWord end = (offset + partition_size (allocation_unit)) / MIN_PARTITION_SIZE;
	UWord cnt = end - idx;
	UWord i0 = idx / sm_table_block_size;
	UWord i1 = idx % sm_table_block_size;
	Partition** pblock = sm_part_table + i0;
	if (sparse_table (table_bytes ()))
		g_protection_domain_memory->commit (pblock, (end / sm_table_block_size - i0) * sizeof (Partition*));

	Partition* ret = 0;
	UWord block_size = sm_table_block_size * sizeof (Partition);
	for (;;) {
		Partition* block = atomic_load ((volatile atomic <Partition*>*)pblock);
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
		if (!ret)
			ret = p;
		UWord tail = sm_table_block_size - i1;
		if (tail > cnt)
			tail = cnt;
		if (commit)
			g_protection_domain_memory->commit (p, tail * sizeof (Partition));
		Partition* end = p + tail;
		do {
			p->set (part, allocation_unit);
			--cnt;
		} while (end != ++p);
		if (cnt) {
			++pblock;
			i1 = 0;
		} else
			break;
	}
	return *ret;
}

void Heap64::remove_partition (Partition& part)
{
	UWord offset = (Octet*)part.directory () - sm_space_begin;
	UWord idx = offset / MIN_PARTITION_SIZE;
	UWord end = (offset + partition_size (part.allocation_unit ())) / MIN_PARTITION_SIZE;
	UWord cnt = end - idx;
	UWord i0 = idx / sm_table_block_size;
	UWord i1 = idx % sm_table_block_size;
	Partition** pblock = sm_part_table + i0;
	for (;;) {
		UWord tail = sm_table_block_size - i1;
		if (tail > cnt)
			tail = cnt;
		Partition* p = *pblock + i1;
		Partition* end = p + tail;
		do {
			p->clear ();
			--cnt;
		} while (end != ++p);

		if (cnt) {
			++pblock;
			i1 = 0;
		} else
			break;
	}
}

const HeapBase::Partition* Heap64::partition (UWord idx)
{
	UWord i0 = idx / sm_table_block_size;
	UWord i1 = idx % sm_table_block_size;
	Partition* const* pblock = sm_part_table + i0;
	if (!sparse_table (table_bytes ()) || g_protection_domain_memory->is_readable (pblock, sizeof (Partition*))) {
		const Partition* p = *pblock;
		if (p) {
			p += i1;
			if (sm_table_block_size * sizeof (Partition) <= sm_commit_unit || g_protection_domain_memory->is_readable (p, sizeof (Partition)))
				return p;
		}
	}
	return 0;
}

Heap::Partition& Heap::create_partition () const
{
	Directory* dir = HeapBase::create_partition (m_allocation_unit);
	try {
		return add_partition (dir, m_allocation_unit);
	} catch (...) {
		g_protection_domain_memory->release (dir, partition_size (m_allocation_unit));
		throw;
	}
}

void Heap::destroy_partition (Partition& part)
{
	Directory* dir = part.directory ();
	UWord au = part.allocation_unit ();
	remove_partition (part);
	g_protection_domain_memory->release (dir, partition_size (au));
}

const HeapBase::Partition* Heap::get_partition (const void* address)
{
	if (!valid_address (address))
		throw BAD_PARAM ();
	UWord off = (Octet*)address - sm_space_begin;
	UWord idx = off / MIN_PARTITION_SIZE;
	UWord rem = off % MIN_PARTITION_SIZE;
	UWord prev = idx ? idx - 1 : 0;
	if (rem < sizeof (Directory))
		--idx;

	for (;;) {
		const Partition* p = partition (idx);
		if (p && p->contains (address))
			return p;

		if (idx == prev)
			break;
		--idx;
	}
	return 0;
}

Heap::~Heap ()
{
	for (Partition* p = m_part_list; p;) {
		Partition* next = p->next;
		destroy_partition (*p);
		p = next;
	}
}

Pointer Heap::allocate (UWord size)
{
	Partition* last_part;
	Pointer p = HeapBase::allocate (size, last_part);
	if (!p) {
		do {
			Partition& new_part = create_partition ();
			bool linked = false;
			try {
				for (;;) {
					Partition* next_part = 0;
					if (atomic_compare_exchange_strong ((volatile atomic <Partition*>*)&last_part->next, &next_part, &new_part)) {
						next_part = &new_part;
						linked = true;
					}
					if (p = next_part->allocate (size))
						break;
					else {
						last_part = next_part;
						if (linked)
							break;
					}
				}
			} catch (...) {
				if (!linked)
					destroy_partition (new_part);
				throw;
			}

			if (!linked)
				destroy_partition (new_part);
		} while (!p);
	}

	return p;
}

}
