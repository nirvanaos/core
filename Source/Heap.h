#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_

#include "HeapDirectory.h"
#include "core.h"
#include "config.h"
#include <type_traits>
#include <HeapFactory.h>

namespace Nirvana {
namespace Core {

class HeapBase
{
protected:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_USE_EXCEPTION> Directory;

	static Pointer allocate (Directory* part, UWord size, UWord allocation_unit);

	struct Partition
	{
		void set (Directory* dir, UWord unit)
		{
			assert (!dir_and_unit);
			assert (!next);
			assert (((UWord)dir & (HEAP_UNIT_MAX - 1)) == 0);
			dir_and_unit = (UWord)dir | (unit - 1);
		}

		UWord exists () const
		{
			return dir_and_unit;
		}

		void clear ()
		{
			dir_and_unit = 0;
			next = 0;
		}

		Directory* directory () const
		{
			return (Directory*)(dir_and_unit & ~(UWord)(HEAP_UNIT_MAX - 1));
		}

		UWord allocation_unit () const
		{
			return (dir_and_unit & (HEAP_UNIT_MAX - 1)) + 1;
		}

		Pointer allocate (UWord size) const
		{
			return HeapBase::allocate (directory (), size, allocation_unit ());
		}

		bool allocate (Pointer p, UWord size, Flags flags) const;

		void release (Pointer p, UWord size) const
		{
			Directory* dir = directory ();
			Octet* heap = (Octet*)(dir + 1);
			UWord offset = (Octet*)p - heap;
			UWord au = allocation_unit ();
			UWord begin = offset / au;
			UWord end = (offset + size + au - 1) / au;
			if (!dir->check_allocated (begin, end, protection_domain_memory ()))
				throw BAD_PARAM ();
			HeapInfo hi = {heap, au, optimal_commit_unit_};
			dir->release (begin, end, protection_domain_memory (), &hi);
		}

		bool contains (const void* p) const
		{
			Octet* begin = (Octet*)(directory () + 1);
			Octet* end = begin + allocation_unit () * Directory::UNIT_COUNT;
			return p >= begin && p < end;
		}

		UWord dir_and_unit;
		Partition* next;
	};

	static void initialize ()
	{
		assert (protection_domain_memory ()->query (0, Memory::ALLOCATION_UNIT) >= HEAP_UNIT_MAX);
		commit_unit_ = protection_domain_memory ()->query (0, Memory::COMMIT_UNIT);
		optimal_commit_unit_ = protection_domain_memory ()->query (0, Memory::OPTIMAL_COMMIT_UNIT);
		space_begin_ = (Octet*)protection_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_BEGIN);
		space_end_ = (Octet*)protection_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_END);
	}

	static bool valid_address (const void* p)
	{
		return space_begin_ <= (Octet*)p && (Octet*)p + 1 <= space_end_;
	}

	static UWord partition_size (const UWord allocation_unit)
	{
		return sizeof (Directory) + Directory::UNIT_COUNT * allocation_unit;
	}

	static const UWord MIN_PARTITION_SIZE = sizeof (Directory) + Directory::UNIT_COUNT * HEAP_UNIT_MIN;

	static UWord max_partitions ()
	{
		return ((space_end_ - space_begin_) + MIN_PARTITION_SIZE - 1) / MIN_PARTITION_SIZE;
	}

	HeapBase (ULong allocation_unit);

	static Directory* create_partition (UWord allocation_unit)
	{
		Directory* part = (Directory*)protection_domain_memory ()->allocate (0, partition_size (allocation_unit), Memory::RESERVED | Memory::ZERO_INIT);
		Directory::initialize (part, protection_domain_memory ());
		return part;
	}

	Pointer allocate (UWord size, Partition*& last_part) const
	{
		assert (part_list_);
		for (Partition* part = part_list_;;) {
			Pointer p = part->allocate (size);
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

	static bool sparse_table (UWord table_bytes)
	{
		return ((table_bytes + HEAP_UNIT_DEFAULT - 1) / HEAP_UNIT_DEFAULT > Directory::MAX_BLOCK_SIZE);
	}

protected:
	UWord allocation_unit_;
	Partition* part_list_;

	static UWord commit_unit_;
	static UWord optimal_commit_unit_;
	static Octet* space_begin_;
	static Octet* space_end_;
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
				part_table_ = (Partition*)protection_domain_memory ()->allocate (0, table_size, Memory::RESERVED | Memory::ZERO_INIT);
				large_table = true;
			} else
				part_table_ = (Partition*)allocate (dir0, table_size, HEAP_UNIT_DEFAULT);
			return add_partition (dir0, HEAP_UNIT_DEFAULT);
		} catch (...) {
			if (large_table)
				protection_domain_memory ()->release (part_table_, table_size);
			protection_domain_memory ()->release (dir0, partition_size (HEAP_UNIT_DEFAULT));
			throw;
		}
	}

	static Partition& add_partition (Directory* part, UWord allocation_unit);
	static void remove_partition (Partition& part);

	static const Partition* partition (UWord idx);

	Heap32 (ULong allocation_unit) :
		HeapBase (allocation_unit)
	{}

private:
	static UWord table_bytes ()
	{
		return max_partitions () * sizeof (Partition);
	}

private:
	static Partition* part_table_;
};

class Heap64 : public HeapBase
{
protected:
	static Partition& initialize ()
	{
		HeapBase::initialize ();
		Directory* dir0 = create_partition (HEAP_UNIT_DEFAULT);
		table_block_size_ = protection_domain_memory ()->query (0, Memory::ALLOCATION_UNIT) / sizeof (Partition);
		UWord table_size = table_bytes ();
		bool large_table = false;
		try {
			if (sparse_table (table_size)) {
				part_table_ = (Partition**)protection_domain_memory ()->allocate (0, table_size, Memory::RESERVED | Memory::ZERO_INIT);
				large_table = true;
			} else
				part_table_ = (Partition**)allocate (dir0, table_size, HEAP_UNIT_DEFAULT);
			return add_partition (dir0, HEAP_UNIT_DEFAULT);
		} catch (...) {
			if (large_table)
				protection_domain_memory ()->release (part_table_, table_size);
			protection_domain_memory ()->release (dir0, partition_size (HEAP_UNIT_DEFAULT));
			throw;
		}
	}

	static Partition& add_partition (Directory* part, UWord allocation_unit);
	static void remove_partition (Partition& part);

	static const Partition* partition (UWord idx);

	Heap64 (ULong allocation_unit) :
		HeapBase (allocation_unit)
	{}

private:
	static UWord table_bytes ()
	{
		return max_partitions () / table_block_size_ * sizeof (Partition*);
	}

private:
	static UWord table_block_size_;
	static Partition** part_table_;
};

typedef std::conditional <(sizeof (UWord) > 4), Heap64, Heap32>::type HeapBaseT;

class HeapFactoryImpl :
	public ::CORBA::Nirvana::ServantStatic <HeapFactoryImpl, HeapFactory>
{
public:
	static Memory_ptr create ();
	static Memory_ptr create_with_granularity (ULong granularity);
};

class Heap :
	public HeapBaseT,
	public ::CORBA::Nirvana::Servant <Heap, Memory>
{
public:
	Pointer allocate (Pointer p, UWord size, Flags flags);

	static void release (Pointer p, UWord size)
	{
		const Partition* part = get_partition (p);
		if (part)
			part->release (p, size);
		else
			protection_domain_memory ()->release (p, size);
	}

	static void commit (Pointer p, UWord size)
	{
		protection_domain_memory ()->commit (p, size);
	}

	static void decommit (Pointer p, UWord size)
	{
		if (!get_partition (p))
			protection_domain_memory ()->decommit (p, size);
	}

	Pointer copy (Pointer dst, Pointer src, UWord size, Flags flags);

	static Boolean is_readable (ConstPointer p, UWord size)
	{
		return protection_domain_memory ()->is_readable (p, size);
	}

	static Boolean is_writable (ConstPointer p, UWord size)
	{
		return protection_domain_memory ()->is_writable (p, size);
	}

	static Boolean is_private (ConstPointer p, UWord size)
	{
		return protection_domain_memory ()->is_private (p, size);
	}

	static Boolean is_copy (ConstPointer p1, ConstPointer p2, UWord size)
	{
		return protection_domain_memory ()->is_copy (p1, p2, size);
	}

	Word query (ConstPointer p, Memory::QueryParam param);

	static void initialize ()
	{
		Partition& first_part = HeapBaseT::initialize ();
		Heap* instance = (Heap*)first_part.allocate (sizeof (Heap));
		new (instance) Heap (first_part);
		g_core_heap = instance;
		g_heap_factory = HeapFactoryImpl::_this ();
	}

	Heap (ULong allocation_unit) :
		HeapBaseT (allocation_unit),
		no_destroy_ (false)
	{
		part_list_ = &create_partition ();
	}

	~Heap ();

	void _add_ref ()
	{
		if (!no_destroy_)
			::CORBA::Nirvana::Servant <Heap, Memory>::_add_ref ();
	}

	void _remove_ref ()
	{
		if (!no_destroy_)
			::CORBA::Nirvana::Servant <Heap, Memory>::_remove_ref ();
	}

	void* operator new (size_t cb)
	{
		return g_core_heap->allocate (0, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		g_core_heap->release (p, cb);
	}

private:
	void* operator new (size_t cb, void* p)
	{
		return p;
	}

	void operator delete (void* p, void* p1)
	{}

	Heap (Partition& first_part) :
		HeapBaseT (HEAP_UNIT_DEFAULT),
		no_destroy_ (true)
	{
		part_list_ = &first_part;
	}

private:
	Pointer allocate (UWord size) const;
	Partition& create_partition () const;
	static const Partition* get_partition (const void* address);
	static void destroy_partition (Partition& part);

private:
	bool no_destroy_;
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
			if (!part->allocate (p, size, flags))
				if (flags & Memory::EXACTLY)
					return 0;
				else
					p = 0;
		} else if (
			!(p = protection_domain_memory ()->allocate (p, size, flags | Memory::EXACTLY))
			&&
			(flags & Memory::EXACTLY)
			)
			return 0;
	}

	if (!p) {
		if (size > Directory::MAX_BLOCK_SIZE * allocation_unit_ || ((flags & Memory::RESERVED) && size >= optimal_commit_unit_))
			p = protection_domain_memory ()->allocate (0, size, flags);
		else {
			try {
				p = allocate (size);
			} catch (const NO_MEMORY&) {
				if (flags & Memory::EXACTLY)
					return 0;
				throw;
			}
			if (flags & Memory::ZERO_INIT)
				zero ((Word*)p, (Word*)p + (size + sizeof (Word) - 1) / sizeof (Word));
		}
	}

	return p;
}

inline Pointer Heap::copy (Pointer dst, Pointer src, UWord size, Flags flags)
{
	if (!size)
		return dst;

	if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
		throw INV_FLAG ();

	if (dst && (flags & Memory::ALLOCATE)) {
		const Partition* part = get_partition (dst);
		if (part) {
			if (part->allocate (dst, size, flags))
				flags &= ~Memory::ALLOCATE;
			else if (flags & Memory::EXACTLY)
				return 0;
		}
	}

	return protection_domain_memory ()->copy (dst, src, size, flags);
}

inline Word Heap::query (ConstPointer p, Memory::QueryParam param)
{
	if (Memory::ALLOCATION_UNIT == param) {
		if (!p)
			return allocation_unit_;
		const Partition* part = get_partition (p);
		if (part)
			return part->allocation_unit ();
	}
	return protection_domain_memory ()->query (p, param);
}

inline Memory_ptr HeapFactoryImpl::create ()
{
	return new Heap (HEAP_UNIT_DEFAULT);
}

inline Memory_ptr HeapFactoryImpl::create_with_granularity (ULong granularity)
{
	return new Heap (granularity);
}

}
}

#endif
