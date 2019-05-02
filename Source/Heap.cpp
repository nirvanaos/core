#include "Heap.h"
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace std;

UWord HeapBase::commit_unit_;
UWord HeapBase::optimal_commit_unit_;
Octet* HeapBase::space_begin_;
Octet* HeapBase::space_end_;

HeapBase::Partition* Heap32::part_table_;

UWord Heap64::table_block_size_;
HeapBase::Partition** Heap64::part_table_;

Pointer HeapBase::allocate (Directory* part, UWord size, UWord allocation_unit)
{
	UWord units = (size + allocation_unit - 1) / allocation_unit;
	HeapInfo hi = {part + 1, allocation_unit, optimal_commit_unit_};
	Word unit = part->allocate (units, &hi);
	if (unit >= 0) {
		assert (unit < Directory::UNIT_COUNT);
		return (Octet*)(part + 1) + unit * allocation_unit;
	}
	return 0;
}

HeapBase::HeapBase (ULong allocation_unit) :
	part_list_ (0)
{
	if (allocation_unit <= HEAP_UNIT_MIN)
		allocation_unit_ = HEAP_UNIT_MIN;
	else if (allocation_unit >= HEAP_UNIT_MAX)
		allocation_unit_ = HEAP_UNIT_MAX;
	else
		allocation_unit_ = clp2 (allocation_unit);
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

	HeapInfo hi = {heap, au, optimal_commit_unit_};
	if (dir->allocate (begin, end, &hi)) {
		Octet* pbegin = heap + begin * au;
		if (!(flags & Memory::EXACTLY))
			p = pbegin;

		if (flags & Memory::ZERO_INIT)
			zero ((Word*)pbegin, (Word*)(heap + end * au));

		return true;
	}
	return false;
}

HeapBase::Partition& Heap32::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	UWord offset = (Octet*)part - space_begin_;
	Partition* begin = part_table_ + offset / MIN_PARTITION_SIZE;
	Partition* end = part_table_ + (offset + partition_size (allocation_unit)) / MIN_PARTITION_SIZE;
	if (sparse_table (table_bytes ()))
		Port::ProtDomainMemory::commit (begin, (end - begin) * sizeof (Partition));
	for (Partition* p = begin; p != end; ++p)
		p->set (part, allocation_unit);
	return *begin;
}

void Heap32::remove_partition (Partition& part)
{
	Partition* begin = &part;
	Partition* end = part_table_ + ((Octet*)begin->directory () + partition_size (begin->allocation_unit ()) - space_begin_) / MIN_PARTITION_SIZE;
	for (Partition* p = begin; p != end; ++p)
		p->clear ();
}

inline const HeapBase::Partition* Heap32::partition (UWord idx)
{
	const Partition* p = part_table_ + idx;
	if (HEAP_DIRECTORY_USE_EXCEPTION) {
		try {
			if (p->exists ())
				return p;
		} catch (...) {
		}
	} else {
		if (
			(!sparse_table (table_bytes ()) || Port::ProtDomainMemory::is_readable (p, sizeof (Partition)))
		&&
			p->exists ()
		)
			return p;
	}
	return 0;
}

HeapBase::Partition& Heap64::add_partition (Directory* part, UWord allocation_unit)
{
	assert (valid_address (part));
	UWord offset = (Octet*)part - space_begin_;
	UWord idx = offset / MIN_PARTITION_SIZE;
	UWord end = (offset + partition_size (allocation_unit)) / MIN_PARTITION_SIZE;
	UWord cnt = end - idx;
	UWord i0 = idx / table_block_size_;
	UWord i1 = idx % table_block_size_;
	Partition** pblock = part_table_ + i0;
	if (sparse_table (table_bytes ()))
		Port::ProtDomainMemory::commit (pblock, ((end + table_block_size_ - 1) / table_block_size_ - i0) * sizeof (Partition*));

	Partition* ret = 0;
	UWord block_size = table_block_size_ * sizeof (Partition);
	for (;;) {
		Partition* block = atomic_load ((volatile atomic <Partition*>*)pblock);
		bool commit = false;
		if (!block) {
			commit = true;
			block = (Partition*)Port::ProtDomainMemory::allocate (0, block_size, Memory::RESERVED | Memory::ZERO_INIT);
			Partition* cur = 0;
			if (!atomic_compare_exchange_strong ((volatile atomic <Partition*>*)pblock, &cur, block)) {
				Port::ProtDomainMemory::release (block, block_size);
				block = cur;
			}
		} else
			commit = block_size > commit_unit_;

		Partition* p = block + i1;
		if (!ret)
			ret = p;
		UWord tail = table_block_size_ - i1;
		if (tail > cnt)
			tail = cnt;
		if (commit)
			Port::ProtDomainMemory::commit (p, tail * sizeof (Partition));
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
	UWord offset = (Octet*)part.directory () - space_begin_;
	UWord idx = offset / MIN_PARTITION_SIZE;
	UWord end = (offset + partition_size (part.allocation_unit ())) / MIN_PARTITION_SIZE;
	UWord cnt = end - idx;
	UWord i0 = idx / table_block_size_;
	UWord i1 = idx % table_block_size_;
	Partition** pblock = part_table_ + i0;
	for (;;) {
		UWord tail = table_block_size_ - i1;
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
	UWord i0 = idx / table_block_size_;
	UWord i1 = idx % table_block_size_;
	Partition* const* pblock = part_table_ + i0;
	if (HEAP_DIRECTORY_USE_EXCEPTION) {
		try {
			const Partition* p = (*pblock) + i1;
			if (p->exists ())
				return p;
		} catch (...) {
		}
	} else {
		if (!sparse_table (table_bytes ()) || Port::ProtDomainMemory::is_readable (pblock, sizeof (Partition*))) {
			const Partition* p = *pblock;
			if (p) {
				p += i1;
				if (
					(
						table_block_size_ * sizeof (Partition) <= commit_unit_
						||
						Port::ProtDomainMemory::is_readable (p, sizeof (Partition))
					)
					&& p->exists ()
				)
					return p;
			}
		}
	}
	return 0;
}

Heap::Partition& Heap::create_partition () const
{
	Directory* dir = HeapBase::create_partition (allocation_unit_);
	try {
		return add_partition (dir, allocation_unit_);
	} catch (...) {
		Port::ProtDomainMemory::release (dir, partition_size (allocation_unit_));
		throw;
	}
}

void Heap::destroy_partition (Partition& part)
{
	Directory* dir = part.directory ();
	UWord au = part.allocation_unit ();
	remove_partition (part);
	Port::ProtDomainMemory::release (dir, partition_size (au));
}

const HeapBase::Partition* Heap::get_partition (const void* address)
{
	if (!valid_address (address))
		throw BAD_PARAM ();
	UWord off = (Octet*)address - space_begin_;
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
	for (Partition* p = part_list_; p;) {
		Partition* next = p->next;
		destroy_partition (*p);
		p = next;
	}
}

Pointer Heap::allocate (UWord size) const
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
}
