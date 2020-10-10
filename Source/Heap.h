#ifndef NIRVANA_CORE_HEAP_H_
#define NIRVANA_CORE_HEAP_H_

#include "HeapDirectory.h"
#include "SkipList.h"

namespace Nirvana {
namespace Core {

template <class T>
class CoreAllocator :
	public std::allocator <T>
{
public:
	void deallocate (T* p, size_t cnt);
	T* allocate (size_t cnt, void* hint = nullptr);
};

class Heap
{
	// Skip list level count for heap skip list.
	// May be moved to Port/config.h
	static const unsigned SKIP_LIST_LEVELS = 10;

public:
	static void initialize ();
	static void terminate ();

	Heap (size_t allocation_unit) NIRVANA_NOEXCEPT;

	~Heap ()
	{
		cleanup ();
	}

	void cleanup ();

	void* allocate (void* p, size_t size, UWord flags);
	void release (void* p, size_t size);

	void commit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::ProtDomainMemory::commit (p, size);
	}

	void decommit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::ProtDomainMemory::decommit (p, size);
	}

	void* copy (void* dst, void* src, size_t size, UWord flags);

	static bool is_readable (const void* p, size_t size)
	{
		return Port::ProtDomainMemory::is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return Port::ProtDomainMemory::is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return Port::ProtDomainMemory::is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return Port::ProtDomainMemory::is_copy (p1, p2, size);
	}

	uintptr_t query (const void* p, MemQuery param);

protected:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HeapDirectoryImpl::COMMITTED_BITMAP> Directory;
	
	// MemoryBlock represents the heap partition or large memory block allocated from the main memory service.
	class MemoryBlock
	{
	public:
		MemoryBlock () NIRVANA_NOEXCEPT
		{}

		MemoryBlock (const void* p) NIRVANA_NOEXCEPT :
			begin_ ((uint8_t*)p)
		{}

		MemoryBlock (void* p, size_t size) NIRVANA_NOEXCEPT :
			begin_ ((uint8_t*)p),
			large_block_size_ (size | 1)
		{
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
			else if (is_large_block () && !large_block_size ())
				return false;
			else if (rhs.is_large_block () && !rhs.large_block_size ())
				return true;
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
			size_t large_block_size_;
		};
	};

	class BlockList : public SkipList <MemoryBlock, SKIP_LIST_LEVELS>
	{
		typedef SkipList <MemoryBlock, SKIP_LIST_LEVELS> Base;
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
			auto ins = Base::insert (new (Heap::allocate (*part, Base::node_size (level), 0, allocation_unit)) NodeVal (level, part));
			assert (ins.second);
			return ins.first;
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

	static Directory* create_partition (size_t allocation_unit)
	{
		return new (Port::ProtDomainMemory::allocate (0, sizeof (Directory) + partition_size (allocation_unit), 
			Directory::IMPLEMENTATION > HeapDirectoryImpl::COMMITTED_BITMAP ? Memory::RESERVED : Memory::ZERO_INIT)
			) Directory ();
	}

	Directory* create_partition () const
	{
		return create_partition (allocation_unit_);
	}

	void release_partition (Directory* dir)
	{
		Port::ProtDomainMemory::release (dir, sizeof (Directory) + partition_size (allocation_unit_));
	}

	Directory* get_partition (const void* p) NIRVANA_NOEXCEPT;

	static void* allocate (Directory& part, size_t size, UWord flags, size_t allocation_unit) NIRVANA_NOEXCEPT;

	void* allocate (Directory& part, size_t size, UWord flags) const NIRVANA_NOEXCEPT
	{
		return allocate (part, size, flags, allocation_unit_);
	}

	void* allocate (Directory& part, void* p, size_t size, UWord flags) const NIRVANA_NOEXCEPT;

	void* allocate (size_t size, UWord flags);

	void release (Directory& part, void* p, size_t size) const;

	virtual MemoryBlock* add_new_partition (MemoryBlock*& tail);

	void add_large_block (void* p, size_t size);

	bool check_owner (const void* p, size_t size);

	static uintptr_t large_allocation_unit (const void* p)
	{
		if (Port::ProtDomainMemory::FIXED_ALLOCATION_UNIT)
			return Port::ProtDomainMemory::FIXED_ALLOCATION_UNIT;
		else
			return Port::ProtDomainMemory::query (p, MemQuery::ALLOCATION_UNIT);
	}

	/// \summary Atomically erase large block information from the block list.
	class LBErase
	{
	public:
		/// \param block_list The block list.
		/// \param first_node First large block node found in the block list.
		/// \param p Begin of the released memory.
		/// \param size Size of the released memory.
		LBErase (BlockList& block_list, BlockList::NodeVal* first_node);
		~LBErase ();

		void prepare (void* p, size_t size);
		void commit () NIRVANA_NOEXCEPT;
		void rollback () NIRVANA_NOEXCEPT;

	private:
		void rollback_helper () NIRVANA_NOEXCEPT;

		struct LBNode
		{
			BlockList::NodeVal* node;
			size_t size;

			bool collapse () NIRVANA_NOEXCEPT
			{
				return node->value ().collapse_large_block (size);
			}

			void restore () NIRVANA_NOEXCEPT
			{
				node->value ().restore_large_block (size);
			}

			void restore (size_t new_size) NIRVANA_NOEXCEPT
			{
				node->value ().restore_large_block (new_size);
			}
		};

		BlockList& block_list_;
		LBNode first_block_, last_block_;
		typedef std::vector <LBNode, CoreAllocator <LBNode> > MiddleBlocks;
		MiddleBlocks middle_blocks_;
		BlockList::NodeVal* new_node_;
		size_t shrink_size_;
	};

protected:
	size_t allocation_unit_;
	BlockList block_list_;
	MemoryBlock* part_list_;
};

class CoreHeap : public Heap
{
public:
	CoreHeap () :
		Heap (HEAP_UNIT_CORE)
	{}

private:
	virtual MemoryBlock* add_new_partition (MemoryBlock*& tail);
};

extern Heap* g_core_heap;

template <class T> inline
void CoreAllocator <T>::deallocate (T* p, size_t cnt)
{
	g_core_heap->release (p, cnt * sizeof (T));
}

template <class T> inline
T* CoreAllocator <T>::allocate (size_t cnt, void* hint)
{
	return (T*)g_core_heap->allocate (0, cnt * sizeof (T), 0);
}

}
}

#endif
