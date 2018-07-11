#include "Heap.h"
#include <atomic>

namespace Nirvana {

using namespace std;

HeapBase::Partition& Heap32::add_partition (Directory* part)
{
	assert (valid_address (part));
	Partition* p = m_part_table + ((Octet*)part - m_space_begin) / partition_size ();
	if (m_size > m_commit_unit)
		prot_domain_memory ()->commit (p, sizeof (Partition));
	assert (!p->directory);
	p->directory = part;
	assert (!p->next);
	return *p;
}

HeapBase::Directory* Heap32::get_partition (const void* address) const
{
	if (!valid_address (address))
		throw BAD_PARAM ();
	const Partition* p = m_part_table + ((Octet*)address - m_space_begin) / partition_size ();
	if (m_size <= m_commit_unit || prot_domain_memory ()->is_readable (p, sizeof (Partition)))
		return p->directory;

	return 0;
}

HeapBase::Partition& Heap64::add_partition (Directory* part)
{
	assert (valid_address (part));
	UWord idx = ((Octet*)part - m_space_begin) / partition_size ();
	UWord i0 = idx / m_table_block_size;
	UWord i1 = idx % m_table_block_size;
	Partition** pblock = m_part_table + i0;
	if (m_size > m_commit_unit)
		prot_domain_memory ()->commit (pblock, sizeof (Partition*));
	Partition* block = atomic_load ((volatile atomic <Partition*>*)pblock);
	UWord block_size = m_table_block_size * sizeof (Partition);
	bool commit = false;
	if (!block) {
		commit = true;
		block = (Partition*)prot_domain_memory ()->allocate (0, block_size, Memory::RESERVED | Memory::ZERO_INIT);
		Partition* cur = 0;
		if (!atomic_compare_exchange_strong ((volatile atomic <Partition*>*)pblock, &cur, block)) {
			prot_domain_memory ()->release (block, block_size);
			block = cur;
		}
	} else
		commit = block_size < m_commit_unit;

	Partition* p = block + i1;
	if (commit)
		prot_domain_memory ()->commit (p, sizeof (Partition));
	assert (!p->directory);
	p->directory = part;
	assert (!p->next);
	return *p;
}

HeapBase::Directory* Heap64::get_partition (const void* address) const
{
	if (!valid_address (address))
		throw BAD_PARAM ();

	UWord idx = ((Octet*)address - m_space_begin) / partition_size ();
	UWord i0 = idx / m_table_block_size;
	UWord i1 = idx % m_table_block_size;
	Partition* const* pblock = m_part_table + i0;
	if (m_size <= m_commit_unit || prot_domain_memory ()->is_readable (pblock, sizeof (Partition*))) {
		Partition* p = *pblock + i1;
		if (m_table_block_size * sizeof (Partition) <= m_commit_unit || prot_domain_memory ()->is_readable (p, sizeof (Partition)))
			return p->directory;
	}

	return 0;
}

Pointer Heap::allocate (UWord size)
{
	Partition* last_part;
	Pointer p = HeapBase::allocate (size, last_part);
	if (!p) {
		Directory* dir = create_partition ();
		try {
			Partition* new_part = &add_partition (dir);
			for (;;) {
				Partition* next_part = 0;
				if (atomic_compare_exchange_strong ((volatile atomic <Partition*>*)&last_part->next, &next_part, new_part))
					next_part = new_part;
				if (p = HeapBase::allocate (next_part->directory, size)) {
					if (next_part != new_part) {
						new_part->directory = 0;
						prot_domain_memory ()->release (dir, partition_size ());
					}
					break;
				} else
					last_part = next_part;
			}
		} catch (...) {
			prot_domain_memory ()->release (dir, partition_size ());
			throw;
		}
	}

	return p;
}

}
