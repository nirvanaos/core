#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_

#include "HeapDirectory.h"
#include "core.h"
#include <type_traits>

namespace Nirvana {

class HeapBase
{
public:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_USE_EXCEPTION> Directory;

	HeapBase (ULong allocation_unit) :
		m_allocation_unit (allocation_unit),
		m_commit_unit (prot_domain_memory ()->query (0, Memory::COMMIT_UNIT)),
		m_optimal_commit_unit (prot_domain_memory ()->query (0, Memory::OPTIMAL_COMMIT_UNIT)),
		m_space_begin ((Octet*)prot_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_BEGIN)),
		m_space_end ((Octet*)prot_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_END))
	{
		assert (allocation_unit);
	}

	struct Partition
	{
		Directory* directory;
		Partition* next;
	};

	UWord partition_size () const
	{
		return sizeof (Directory) + Directory::UNIT_COUNT * m_allocation_unit;
	}

	UWord max_partitions () const
	{
		return (m_space_end - m_space_begin) / partition_size ();
	}

	Directory* create_partition (Flags flags = 0) const
	{
		Directory* part = (Directory*)prot_domain_memory ()->allocate (0, partition_size (), flags | Memory::RESERVED | Memory::ZERO_INIT);
		Directory::initialize (part, prot_domain_memory ());
		return part;
	}

	Pointer allocate (Directory* part, UWord size) const
	{
		Word unit = part->allocate (size, prot_domain_memory ());
		if (unit >= 0)
			return (Octet*)(part + 1) + unit * m_allocation_unit;
		return 0;
	}

	Pointer allocate (UWord size, Partition*& last_part)
	{
		for (Partition* part = m_part_list;;) {
			Pointer p = allocate (part->directory, size);
			if (p)
				return p;
			Partition* next = part->next;
			if (next)
				part = next;
			else {
				last_part = part;
				return 0;
			}
		}
	}

	void release (Directory* part, Pointer p, UWord size) const
	{
		Octet* heap = (Octet*)(part + 1);
		UWord offset = (Octet*)p - heap;
		UWord begin = offset / m_allocation_unit;
		UWord end = (offset + size) / m_allocation_unit;
		if (!part->check_allocated (begin, end, prot_domain_memory ()))
			throw BAD_PARAM ();
		part->release (begin, end, prot_domain_memory (), false, heap, m_allocation_unit);
	}

	void destroy ()
	{
		Directory* first = m_part_list->directory;
		UWord part_size = partition_size ();
		for (Partition* p = m_part_list->next; p; p = p->next)
			prot_domain_memory ()->release (p->directory, part_size);
		prot_domain_memory ()->release (first, part_size);

		if ((Octet*)this < (Octet*)first || (Octet*)first + partition_size () <= (Octet*)this)
			prot_domain_memory ()->release (this, m_size);
	}

	bool valid_address (const void* p) const
	{
		return m_space_begin <= (Octet*)p && (Octet*)p + 1 <= m_space_end;
	}

protected:
	UWord m_size;
	UWord m_allocation_unit;
	UWord m_commit_unit;
	UWord m_optimal_commit_unit;
	Octet* m_space_begin;
	Octet* m_space_end;
	Partition* m_part_list;
};

class Heap32 : public HeapBase
{
protected:
	static Heap32* create (ULong allocation_unit = HEAP_UNIT_MIN)
	{
		HeapBase base (allocation_unit);
		Directory* first_part = base.create_partition ();
		UWord mysize = sizeof (Heap32) + (base.max_partitions () - 1) * sizeof (Partition);
		Heap32* myptr;
		if ((mysize + allocation_unit - 1) / allocation_unit > Directory::MAX_BLOCK_SIZE) {
			try {
				myptr = (Heap32*)prot_domain_memory ()->allocate (0, mysize, Memory::RESERVED | Memory::ZERO_INIT);
			} catch (...) {
				prot_domain_memory ()->release (first_part, base.partition_size ());
				throw;
			}
		} else
			myptr = (Heap32*)base.allocate (first_part, mysize);
		prot_domain_memory ()->commit (myptr, sizeof (HeapBase));
		*static_cast <HeapBase*> (myptr) = base;
		myptr->m_size = mysize;
		myptr->m_part_list = &myptr->add_partition (first_part);
		return myptr;
	}

	Partition& add_partition (Directory* part);

	Directory* get_partition (const void* address) const;

private:
	Partition m_part_table [1];	// Variable length array.
};

class Heap64 : public HeapBase
{
protected:
	static Heap64* create (ULong allocation_unit = HEAP_UNIT_MIN)
	{
		HeapBase base (allocation_unit);
		UWord table_block_size = prot_domain_memory ()->query (0, Memory::ALLOCATION_UNIT) / sizeof (Partition);
		UWord table_size = base.max_partitions () / table_block_size;
		size_t mysize = sizeof (Heap64) + (table_size - 1) * sizeof (Partition*);
		Directory* first_part = base.create_partition ();
		Heap64* myptr;
		if ((mysize + allocation_unit - 1) / allocation_unit > Directory::MAX_BLOCK_SIZE) {
			try {
				myptr = (Heap64*)prot_domain_memory ()->allocate (0, mysize, Memory::RESERVED | Memory::ZERO_INIT);
			} catch (...) {
				prot_domain_memory ()->release (first_part, base.partition_size ());
				throw;
			}
		} else
			myptr = (Heap64*)base.allocate (first_part, mysize);
		prot_domain_memory ()->commit (myptr, sizeof (HeapBase));
		*static_cast <HeapBase*> (myptr) = base;
		myptr->m_size = mysize;
		myptr->m_part_list = &myptr->add_partition (first_part);
		return myptr;
	}

	Partition& add_partition (Directory* part);

	Directory* get_partition (const void* address) const;

private:
	UWord m_table_block_size;
	Partition* m_part_table [1];	// Variable length array.
};

typedef std::conditional <(sizeof (UWord) > 4), Heap64, Heap32>::type HeapBaseT;

class Heap : 
	public HeapBaseT,
	public ::CORBA::Nirvana::Servant <Heap, Memory>
{
public:
	static Heap* create (ULong allocation_unit = HEAP_UNIT_MIN)
	{
		return static_cast <Heap*> (HeapBaseT::create (allocation_unit));
	}

	Pointer allocate (Pointer p, UWord size, Flags flags);

	void release (Pointer p, UWord size)
	{
		Directory* part = get_partition (p);
		if (part)
			HeapBase::release (part, p, size);
	}

	void commit (Pointer p, UWord size)
	{
		prot_domain_memory ()->commit (p, size);
	}

	void decommit (Pointer p, UWord size)
	{
		if (!HeapBaseT::get_partition (p))
			prot_domain_memory ()->decommit (p, size);
	}

	Pointer copy (Pointer dst, Pointer src, UWord size, Flags flags);

	Word query (Pointer p, Memory::QueryParam param);

private:
	Pointer allocate (UWord size);

	Directory* get_partition (const void* address) const
	{
		Directory* part = HeapBaseT::get_partition (address);
		if (part && (part + 1) <= address)
			return part;
		return 0;
	}
};

Pointer Heap::allocate (Pointer p, UWord size, Flags flags)
{
	if (!size)
		throw BAD_PARAM ();

	if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
		throw INV_FLAG ();

	if (size / m_allocation_unit <= Directory::MAX_BLOCK_SIZE && !(flags & Memory::RESERVED)) {
		if (p) {
			Directory* part = get_partition (p);
			if (part) {
				Octet* heap = (Octet*)(part + 1);
				UWord offset = (Octet*)p - heap;
				UWord begin = offset / m_allocation_unit;
				UWord end;
				if (flags & Memory::EXACTLY)
					end = (offset + size + m_allocation_unit - 1) / m_allocation_unit;
				else
					end = begin + (size + m_allocation_unit - 1) / m_allocation_unit;
				if (part->allocate (begin, end, prot_domain_memory ())) {
					if (!(flags & Memory::EXACTLY))
						p = heap + begin * m_allocation_unit;
				} else if (flags & Memory::EXACTLY)
					return 0;
				else
					p = 0;
			} else
				return prot_domain_memory ()->allocate (p, size, flags);
		}
		
		if (!p) {
			try {
				p = allocate (size);
			} catch (const NO_MEMORY&) {
				if (flags & Memory::EXACTLY)
					return 0;
				throw;
			}
		}

		assert (p);
		Octet* commit_begin = round_down ((Octet*)p, m_optimal_commit_unit);
		try {
			prot_domain_memory ()->commit (commit_begin, round_up ((Octet*)p + size, m_optimal_commit_unit) - commit_begin);
		} catch (...) {
			release (p, size);
			throw;
		}

		return p;

	} else
		return prot_domain_memory ()->allocate (p, size, flags);
}

}

#endif
