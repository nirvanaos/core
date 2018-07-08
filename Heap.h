#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_

#include "HeapDirectory.h"
#include "core.h"
#include <type_traits>

namespace Nirvana {

class HeapBase
{
protected:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_USE_EXCEPTION> Directory;

	HeapBase (ULong allocation_unit) :
		m_allocation_unit (allocation_unit)
	{
		assert (allocation_unit);
		m_space_begin = (Octet*)prot_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_BEGIN);
		m_space_end = (Octet*)prot_domain_memory ()->query (0, Memory::ALLOCATION_SPACE_END);
	}

	struct PartEntry
	{
		Directory* dir;
		PartEntry* next;
	};

	size_t part_size () const
	{
		return sizeof (Directory) + Directory::UNIT_COUNT * m_allocation_unit;
	}

	Directory* create_part () const
	{
		Directory* part = (Directory*)prot_domain_memory ()->allocate (0, part_size (), Memory::RESERVED | Memory::ZERO_INIT);
		Directory::initialize (part, prot_domain_memory ());
		return part;
	}

	void* allocate (Directory* part, UWord size) const
	{
		Word unit = part->allocate (size, prot_domain_memory ());
		if (unit >= 0)
			return (Octet*)(part + 1) + unit * m_allocation_unit;
		return 0;
	}

protected:
	Octet* m_space_begin;
	Octet* m_space_end;
	const ULong m_allocation_unit;
	PartEntry* m_part_list;
};

class Heap32 : public HeapBase
{
protected:
	static Heap32* create (ULong allocation_unit = HEAP_UNIT_MIN)
	{
		HeapBase base (allocation_unit);
		Directory* first_part = base.create_part ();
		UWord hash_table_size = (base.m_space_end - base.m_space_begin) / base.part_size (allocation_unit);
		UWord mysize = sizeof (HeapBase) + hash_table_size * sizeof (PartEntry);
		Heap32* myptr;
		if ((mysize + allocation_unit - 1) / allocation_unit > Directory::MAX_BLOCK_SIZE)
			myptr = (Heap32*)prot_domain_memory ()->allocate (0, mysize, Memory::RESERVED | Memory::ZERO_INIT);
		else
			myptr = (Heap32*)base.allocate (first_part, mysize);
		prot_domain_memory ()->commit (myptr, sizeof (HeapBase));
		*static_cast <HeapBase*> (myptr) = base;
		Directory*& dir = myptr->directory (first_part);
		prot_domain_memory ()->commit (&dir, sizeof (PartEntry));
		dir = first_part;
		return myptr;
	}

	void destroy ()
	{

	}

	Directory*& directory (void* address)
	{
		return m_hash_table [((Octet*)address - m_space_begin) / part_size ()].dir;
	}

private:
	PartEntry m_hash_table [1];	// Variable length array.
};

class Heap64 : public HeapBase
{
protected:
	static Heap64* create (ULong allocation_unit = HEAP_UNIT_MIN);

private:

	PartEntry* m_hash_table [1];	// Variable length array.
};

typedef std::conditional <(sizeof (UWord) > 4), Heap64, Heap32> HeapBaseT;

class Heap : 
	public HeapBaseT,
	public ::CORBA::Nirvana::Servant <Heap, Memory>
{
public:
	static Heap* create (ULong allocation_unit);
};

}

#endif
