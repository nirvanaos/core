/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_
#pragma once

#include "HeapDirectory.h"
#include <Port/config.h>
#include "SkipList.h"
#include "CoreInterface.h"
#include "StaticallyAllocated.h"

#define DEFINE_ALLOCATOR(Name) template <class U> operator const Name <U>& () const \
NIRVANA_NOEXCEPT { return *reinterpret_cast <const Name <U>*> (this); }\
template <class U> struct rebind { typedef Name <U> other; }

namespace Nirvana {
namespace Core {

class HeapCore;
class HeapUser;

/// \brief Heap implementation.
///
/// The Heap is lock-free object and can be shared a number of threads.
class Heap
{
	Heap (const Heap&) = delete;
	Heap& operator = (const Heap&) = delete;

	DECLARE_CORE_INTERFACE

public:
	/// See Nirvana::Memory::allocate
	void* allocate (void* p, size_t& size, unsigned flags);

	/// See Nirvana::Memory::release
	void release (void* p, size_t size);

	/// See Nirvana::Memory::commit
	void commit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::Memory::commit (p, size);
	}

	/// See Nirvana::Memory::decommit
	void decommit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::Memory::decommit (p, size);
	}

	/// See Nirvana::Memory::copy
	void* copy (void* dst, void* src, size_t& size, unsigned flags);

	/// See Nirvana::Memory::is_private
	static bool is_private (const void* p, size_t size)
	{
		return Port::Memory::is_private (p, size);
	}

	/// See Nirvana::Memory::query
	uintptr_t query (const void* p, Memory::QueryParam param);

	/// Changes the entire heap protection.
	/// 
	/// \param read_only If `true`, makes this heap read-only.
	///   Otherwise, makes this heap read-write.
	void change_protection (bool read_only);

	/// Checks that memory block is owned by this heap.
	/// 
	/// \param p The memory block pointer.
	/// \param size The memory block size.
	/// \returns `true` if entire memory block is owned by this heap.
	bool check_owner (const void* p, size_t size);

	/// Moves memory block from other heap to this heap.
	/// 
	/// \param other Other heap.
	/// \param p Memory block in \p other.
	/// \param size Memory block size.
	/// \returns Pointer of the memory block in this heap.
	void* move_from (Heap& other, void* p, size_t& size);

	/// Add large block
	///
	/// \param p Memory block in \p other.
	/// \param size Memory block size.
	/// 
	/// On exception, memory block will be released, then exception rethrown.
	void add_large_block (void* p, size_t size);

	/// Returns core heap.
	static Heap& core_heap () NIRVANA_NOEXCEPT;

	/// Returns shared heap.
	static Heap& shared_heap () NIRVANA_NOEXCEPT;

	/// Global class initialization.
	static bool initialize () NIRVANA_NOEXCEPT;
	
	/// Global class temination.
	static void terminate () NIRVANA_NOEXCEPT;

protected:
	Heap (size_t allocation_unit = HEAP_UNIT_DEFAULT) NIRVANA_NOEXCEPT;

	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_LEVELS,
		(Port::Memory::FLAGS & Memory::SPACE_RESERVATION) ?
		HeapDirectoryImpl::COMMITTED_BITMAP : HeapDirectoryImpl::PLAIN_MEMORY> Directory;
	
	// MemoryBlock represents the heap partition or large memory block allocated from the main memory service.
	class MemoryBlock
	{
	public:
		MemoryBlock () NIRVANA_NOEXCEPT
		{}

		MemoryBlock (const void* p) NIRVANA_NOEXCEPT :
			begin_ ((uint8_t*)p),
			next_partition_ (nullptr)
		{}

		MemoryBlock (void* p, size_t size) NIRVANA_NOEXCEPT :
			begin_ ((uint8_t*)p),
			large_block_size_ (size | 1)
		{
			assert (size);
			assert (!(size & 1));
		}

		MemoryBlock (Directory* part) NIRVANA_NOEXCEPT :
			begin_ ((uint8_t*)(part + 1)),
			next_partition_ (nullptr)
		{}

		uint8_t* begin () const NIRVANA_NOEXCEPT
		{
			return begin_;
		}

		// Use lowest bit as large block tag.
		size_t is_large_block () const NIRVANA_NOEXCEPT
		{
			return large_block_size_ & 1;
		}

		size_t large_block_size () const NIRVANA_NOEXCEPT
		{
			assert (is_large_block ());
			return large_block_size_ & ~1;
		}

		Directory& directory () const NIRVANA_NOEXCEPT
		{
			assert (!is_large_block ());
			return *(Directory*)(begin_ - sizeof (Directory));
		}

		MemoryBlock*& next_partition () NIRVANA_NOEXCEPT
		{
			assert (!is_large_block ());
			return next_partition_;
		}

		// Blocks are sorted in descending order.
		// So we can use lower_bound to search block begin.
		bool operator < (const MemoryBlock& rhs) const NIRVANA_NOEXCEPT
		{
			if (begin_ > rhs.begin_)
				return true;
			else if (begin_ < rhs.begin_)
				return false;
			// Hide collapsed blocks
			else if (large_block_size_ == 1)
				return false;
			else if (rhs.large_block_size_ == 1)
				return true;
			else
				return false;
		}

		bool collapse_large_block (size_t size) NIRVANA_NOEXCEPT;

		void restore_large_block (size_t size) NIRVANA_NOEXCEPT;

	private:
		// Pointer to the memory block.
		// For the heap partition it is pointer to memory space immediately after heap directory.
		uint8_t* begin_;

		union
		{
			MemoryBlock* next_partition_;
			volatile size_t large_block_size_;
		};
	};

	class BlockList : public SkipList <MemoryBlock, HEAP_SKIP_LIST_LEVELS>
	{
		typedef SkipList <MemoryBlock, HEAP_SKIP_LIST_LEVELS> Base;
	public:
		void insert (NodeVal*& node)
		{
			auto ins = Base::insert (node);
			assert (ins.second);
			release_node (ins.first);
			node = nullptr;
		}

		void insert (void* p, size_t size)
		{
			auto ins = Base::insert (p, size);
			assert (ins.second);
			release_node (ins.first);
		}

		NodeVal* insert (Directory* part)
		{
			auto ins = Base::insert (part);
			assert (ins.second);
			return ins.first;
		}

		NodeVal* insert (Directory* part, size_t allocation_unit) NIRVANA_NOEXCEPT
		{
			unsigned level = Base::random_level ();
			size_t cb = Base::node_size (level);
			auto ins = Base::insert (new (Heap::allocate (*part, cb, 0, allocation_unit)) NodeVal (level, part));
			assert (ins.second);
#ifdef _DEBUG
			node_cnt_.increment ();
#endif
			return ins.first;
		}

		const NodeVal* head () const NIRVANA_NOEXCEPT
		{
			return static_cast <NodeVal*> (Base::head ());
		}

		const NodeVal* tail () const NIRVANA_NOEXCEPT
		{
			return static_cast <NodeVal*> (Base::tail ());
		}
	};

	static size_t partition_size (size_t allocation_unit)
	{
		return Directory::UNIT_COUNT * allocation_unit;
	}

	size_t partition_size () const
	{
		return partition_size (allocation_unit_);
	}

	Directory* create_partition () const;

	void release_partition (Directory* dir)
	{
		Port::Memory::release (dir, sizeof (Directory) + partition_size (allocation_unit_));
	}

	Directory* get_partition (const void* p) NIRVANA_NOEXCEPT;

	static void* allocate (Directory& part, size_t& size, unsigned flags, size_t allocation_unit) NIRVANA_NOEXCEPT;

	void* allocate (Directory& part, size_t& size, unsigned flags) const NIRVANA_NOEXCEPT
	{
		return allocate (part, size, flags, allocation_unit_);
	}

	void* allocate (Directory& part, void* p, size_t& size, unsigned flags) const NIRVANA_NOEXCEPT;

	void* allocate (size_t& size, unsigned flags);

	void release (Directory& part, void* p, size_t size) const;

	virtual MemoryBlock* add_new_partition (MemoryBlock*& tail);

	/// \summary Atomically erase large block information from the block list.
	class LBErase;

protected:
	size_t allocation_unit_;
	MemoryBlock* part_list_;
	BlockList block_list_;

private:
	static StaticallyAllocated <ImplStatic <HeapCore> > core_heap_;
	static StaticallyAllocated <ImplStatic <HeapUser> > shared_heap_;
};

class HeapCore : public Heap
{
public:
	HeapCore () :
		Heap (HEAP_UNIT_CORE)
	{}

private:
	virtual MemoryBlock* add_new_partition (MemoryBlock*& tail);
};

}
}

#endif
