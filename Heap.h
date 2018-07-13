#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_

#include "HeapDirectory.h"
#include "core.h"
#include "config.h"
#include <type_traits>

namespace Nirvana {

class HeapBase
{
protected:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_USE_EXCEPTION> Directory;

	struct Partition
	{
		void set (Directory* dir, UWord unit)
		{
			assert (!dir_and_unit);
			assert (!next);
			assert (((UWord)dir & (HEAP_UNIT_MAX - 1)) == 0);
			dir_and_unit = (UWord)dir | (unit - 1);
		}

		void release ()
		{
			Directory* dir = directory ();
			UWord au = allocation_unit ();
			dir_and_unit = 0;
			g_protection_domain_memory->release (dir, partition_size (au));
		}

		Directory* directory () const
		{
			return (Directory*)(dir_and_unit & ~(UWord)(HEAP_UNIT_MAX - 1));
		}

		UWord allocation_unit () const
		{
			return (dir_and_unit & (HEAP_UNIT_MAX - 1)) + 1;
		}

		UWord dir_and_unit;
		Partition* next;
	};

	static void initialize ()
	{
		assert (g_protection_domain_memory->query (0, Memory::ALLOCATION_UNIT) >= HEAP_UNIT_MAX);
		sm_commit_unit = g_protection_domain_memory->query (0, Memory::COMMIT_UNIT);
		sm_optimal_commit_unit = g_protection_domain_memory->query (0, Memory::OPTIMAL_COMMIT_UNIT);
		sm_space_begin = (Octet*)g_protection_domain_memory->query (0, Memory::ALLOCATION_SPACE_BEGIN);
		sm_space_end = (Octet*)g_protection_domain_memory->query (0, Memory::ALLOCATION_SPACE_END);
	}

	static bool valid_address (const void* p)
	{
		return sm_space_begin <= (Octet*)p && (Octet*)p + 1 <= sm_space_end;
	}

	static UWord partition_size (const UWord allocation_unit)
	{
		return sizeof (Directory) + Directory::UNIT_COUNT * allocation_unit;
	}

	static const UWord MIN_PARTITION_SIZE = sizeof (Directory) + Directory::UNIT_COUNT * HEAP_UNIT_MIN;

	static UWord max_partitions ()
	{
		return ((sm_space_end - sm_space_begin) + MIN_PARTITION_SIZE - 1) / MIN_PARTITION_SIZE;
	}

	HeapBase (UWord allocation_unit) :
		m_allocation_unit (allocation_unit),
		m_part_list (0)
	{
		if (allocation_unit < HEAP_UNIT_MIN || allocation_unit > HEAP_UNIT_MAX || allocation_unit != (UWord)1 << ntz (allocation_unit))
			throw BAD_PARAM ();
	}

	~HeapBase ()
	{
		for (Partition* p = m_part_list; p; p = p->next)
			p->release ();
	}

	static Directory* create_partition (UWord allocation_unit)
	{
		Directory* part = (Directory*)g_protection_domain_memory->allocate (0, partition_size (allocation_unit), Memory::RESERVED | Memory::ZERO_INIT);
		Directory::initialize (part, g_protection_domain_memory);
		return part;
	}

	static Pointer allocate (Directory* part, UWord size, UWord allocation_unit);

	static Pointer allocate (const Partition& part, UWord size)
	{
		return allocate (part.directory (), size, part.allocation_unit ());
	}

	Pointer allocate (UWord size, Partition*& last_part)
	{
		assert (m_part_list);
		for (Partition* part = m_part_list;;) {
			Pointer p = allocate (*part, size);
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

	static void release (const Partition& part, Pointer p, UWord size)
	{
		Directory* dir = part.directory ();
		Octet* heap = (Octet*)(dir + 1);
		UWord offset = (Octet*)p - heap;
		UWord allocation_unit = part.allocation_unit ();
		UWord begin = offset / allocation_unit;
		UWord end = (offset + size + allocation_unit - 1) / allocation_unit;
		if (!dir->check_allocated (begin, end, g_protection_domain_memory))
			throw BAD_PARAM ();
		dir->release (begin, end, g_protection_domain_memory, false, heap, allocation_unit);
	}

	static void commit_heap (void* p, UWord size)
	{
		Octet* commit_begin = round_down ((Octet*)p, sm_optimal_commit_unit);
		g_protection_domain_memory->commit (commit_begin, round_up ((Octet*)p + size, sm_optimal_commit_unit) - commit_begin);
	}

	static bool sparse_table (UWord table_bytes)
	{
		return ((table_bytes + HEAP_UNIT_DEFAULT - 1) / HEAP_UNIT_DEFAULT > Directory::MAX_BLOCK_SIZE);
	}

protected:
	UWord m_allocation_unit;
	Partition* m_part_list;

	static UWord sm_commit_unit;
	static UWord sm_optimal_commit_unit;
	static Octet* sm_space_begin;
	static Octet* sm_space_end;
};

class Heap32 : protected HeapBase
{
protected:
	static Partition& initialize ()
	{
		HeapBase::initialize ();
		Directory* dir0 = create_partition (HEAP_UNIT_DEFAULT);
		UWord table_size = table_bytes ();
		bool large_table = false;
		try {
			if (sparse_table (table_size)) {
				sm_part_table = (Partition*)g_protection_domain_memory->allocate (0, table_size, Memory::RESERVED | Memory::ZERO_INIT);
				large_table = true;
			} else
				sm_part_table = (Partition*)allocate (dir0, table_size, HEAP_UNIT_DEFAULT);
			return add_partition (dir0, HEAP_UNIT_DEFAULT);
		} catch (...) {
			if (large_table)
				g_protection_domain_memory->release (sm_part_table, table_size);
			g_protection_domain_memory->release (dir0, partition_size (HEAP_UNIT_DEFAULT));
			throw;
		}
	}

	static Partition& add_partition (Directory* part, UWord allocation_unit);

	static const Partition* get_partition (const void* address);

	Heap32 (UWord allocation_unit) :
		HeapBase (allocation_unit)
	{}

private:
	static UWord table_bytes ()
	{
		return max_partitions () * sizeof (Partition);
	}

private:
	static Partition* sm_part_table;
};

class Heap64 : public HeapBase
{
protected:
	static Partition& initialize ()
	{
		HeapBase::initialize ();
		Directory* dir0 = create_partition (HEAP_UNIT_DEFAULT);
		sm_table_block_size = g_protection_domain_memory->query (0, Memory::ALLOCATION_UNIT) / sizeof (Partition);
		UWord table_size = table_bytes ();
		bool large_table = false;
		try {
			if (sparse_table (table_size)) {
				sm_part_table = (Partition**)g_protection_domain_memory->allocate (0, table_size, Memory::RESERVED | Memory::ZERO_INIT);
				large_table = true;
			} else
				sm_part_table = (Partition**)allocate (dir0, table_size, HEAP_UNIT_DEFAULT);
			return add_partition (dir0, HEAP_UNIT_DEFAULT);
		} catch (...) {
			if (large_table)
				g_protection_domain_memory->release (sm_part_table, table_size);
			g_protection_domain_memory->release (dir0, partition_size (HEAP_UNIT_DEFAULT));
			throw;
		}
	}

	static Partition& add_partition (Directory* part, UWord allocation_unit);

	static const Partition* get_partition (const void* address);

	Heap64 (UWord allocation_unit) :
		HeapBase (allocation_unit)
	{}

private:
	static UWord table_bytes ()
	{
		return max_partitions () / sm_table_block_size * sizeof (Partition*);
	}

private:
	static UWord sm_table_block_size;
	static Partition** sm_part_table;
};

typedef std::conditional <(sizeof (UWord) > 4), Heap64, Heap32>::type HeapBaseT;

class Heap : 
	public HeapBaseT,
	public ::CORBA::Nirvana::Servant <Heap, Memory>
{
public:
	Pointer allocate (Pointer p, UWord size, Flags flags);

	void release (Pointer p, UWord size)
	{
		const Partition* part = get_partition (p);
		if (part)
			HeapBase::release (*part, p, size);
		else
			g_protection_domain_memory->release (p, size);
	}

	void commit (Pointer p, UWord size)
	{
		g_protection_domain_memory->commit (p, size);
	}

	void decommit (Pointer p, UWord size)
	{
		if (!HeapBaseT::get_partition (p))
			g_protection_domain_memory->decommit (p, size);
	}

	Pointer copy (Pointer dst, Pointer src, UWord size, Flags flags)
	{
		throw NO_IMPLEMENT ();
	}

	static Boolean is_readable (ConstPointer p, UWord size)
	{
		return g_protection_domain_memory->is_readable (p, size);
	}

	static Boolean is_writable (ConstPointer p, UWord size)
	{
		return g_protection_domain_memory->is_writable (p, size);
	}

	static Boolean is_private (ConstPointer p, UWord size)
	{
		return g_protection_domain_memory->is_private (p, size);
	}

	static Boolean is_copy (ConstPointer p1, ConstPointer p2, UWord size)
	{
		return g_protection_domain_memory->is_copy (p1, p2, size);
	}

	Word query (ConstPointer p, Memory::QueryParam param)
	{
		return g_protection_domain_memory->query (p, param);
	}

	Heap (UWord allocation_unit) :
		HeapBaseT (allocation_unit),
		m_no_destroy (false)
	{}

	static void initialize ()
	{
		Partition& first_part = HeapBaseT::initialize ();
		Heap* instance = (Heap*)HeapBase::allocate (first_part, sizeof (Heap));
		new (instance) Heap (first_part);
		g_default_heap = instance;
	}

	void _add_ref ()
	{
		if (!m_no_destroy)
			::CORBA::Nirvana::Servant <Heap, Memory>::_add_ref ();
	}

	void _remove_ref ()
	{
		if (!m_no_destroy)
			::CORBA::Nirvana::Servant <Heap, Memory>::_remove_ref ();
	}

private:
	Heap (Partition& first_part) :
		HeapBaseT (HEAP_UNIT_DEFAULT),
		m_no_destroy (true)
	{
		m_part_list = &first_part;
	}

private:
	Pointer allocate (UWord size);

	const Partition* get_partition (const void* address) const
	{
		const Partition* part = HeapBaseT::get_partition (address);
		if (part && (part->directory () + 1) <= address)
			return part;
		return 0;
	}

private:
	bool m_no_destroy;
};

inline Pointer Heap::allocate (Pointer p, UWord size, Flags flags)
{
	if (!size)
		throw BAD_PARAM ();

	if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
		throw INV_FLAG ();

	if (p) {
		const Partition* part = get_partition (p);
		if (part) {
			Directory* dir = part->directory ();
			Octet* heap = (Octet*)(dir + 1);
			UWord offset = (Octet*)p - heap;
			UWord allocation_unit = part->allocation_unit ();
			UWord begin = offset / allocation_unit;
			UWord end;
			if (flags & Memory::EXACTLY)
				end = (offset + size + allocation_unit - 1) / allocation_unit;
			else
				end = begin + (size + allocation_unit - 1) / allocation_unit;
			if (dir->allocate (begin, end, g_protection_domain_memory)) {
				if (!(flags & Memory::EXACTLY))
					p = heap + begin * m_allocation_unit;

				try {
					commit_heap (p, size);
				} catch (...) {
					HeapBase::release (*part, p, size);
					if (flags & Memory::EXACTLY)
						return 0;
					throw;
				}
			} else if (flags & Memory::EXACTLY)
				return 0;
			else
				p = 0;
		} else
			p = g_protection_domain_memory->allocate (p, size, flags);
	}

	if (!p && size / m_allocation_unit <= Directory::MAX_BLOCK_SIZE && !(flags & Memory::RESERVED)) {
		try {
			p = allocate (size);
		} catch (const NO_MEMORY&) {
			if (flags & Memory::EXACTLY)
				return 0;
			throw;
		}
	} else
		p = g_protection_domain_memory->allocate (p, size, flags);

	return p;
}

}

#endif
