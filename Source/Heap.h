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
	static void deallocate (T* p, size_t cnt);
	static T* allocate (size_t cnt, void* hint = nullptr, UWord flags = 0);
};

class HeapBase
{
	// Skip list level count for heap skip list.
	// May be moved to Port/config.h
	static const unsigned SKIP_LIST_LEVELS = 10;

	HeapBase (const HeapBase&) = delete;
	HeapBase& operator = (const HeapBase&) = delete;

public:
	HeapBase (size_t allocation_unit = HEAP_UNIT_DEFAULT) NIRVANA_NOEXCEPT;

	void* allocate (void* p, size_t size, UWord flags);
	void release (void* p, size_t size);

	void commit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::Memory::commit (p, size);
	}

	void decommit (void* p, size_t size)
	{
		if (!check_owner (p, size))
			throw_BAD_PARAM ();
		Port::Memory::decommit (p, size);
	}

	void* copy (void* dst, void* src, size_t size, UWord flags);

	static bool is_readable (const void* p, size_t size)
	{
		return Port::Memory::is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return Port::Memory::is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return Port::Memory::is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return Port::Memory::is_copy (p1, p2, size);
	}

	uintptr_t query (const void* p, MemQuery param);

protected:
	typedef HeapDirectory <HEAP_DIRECTORY_SIZE, HEAP_DIRECTORY_LEVELS, HEAP_DIRECTORY_IMPLEMENTATION> Directory;
	
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
			auto ins = Base::insert (new (HeapBase::allocate (*part, Base::node_size (level), 0, allocation_unit)) NodeVal (level, part));
			assert (ins.second);
#ifdef _DEBUG
			node_cnt_.increment ();
#endif
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
		return new (Port::Memory::allocate (0, sizeof (Directory) + partition_size (allocation_unit), 
			Directory::IMPLEMENTATION > HeapDirectoryImpl::COMMITTED_BITMAP ? Memory::RESERVED : Memory::ZERO_INIT)
			) Directory ();
	}

	Directory* create_partition () const
	{
		return create_partition (allocation_unit_);
	}

	void release_partition (Directory* dir)
	{
		Port::Memory::release (dir, sizeof (Directory) + partition_size (allocation_unit_));
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

class CoreHeap final :
	public HeapBase
{
public:
	CoreHeap () :
		HeapBase (HEAP_UNIT_CORE)
	{}

private:
	virtual MemoryBlock* add_new_partition (MemoryBlock*& tail);
};

class CoreHeapObj
{
public:
	void initialize ()
	{
		assert (!initialized_);
		Port::Memory::initialize ();
		new (object_) CoreHeap ();
#ifdef _DEBUG
		initialized_ = true;
#endif
	}

	void terminate ()
	{
		assert (initialized_);
		((CoreHeap*)object_)->~CoreHeap ();
		Port::Memory::terminate ();
#ifdef _DEBUG
		initialized_ = false;
#endif
	}

	HeapBase* operator -> ()
	{
		return (CoreHeap*)object_;
	}

private:
	uint8_t object_ [sizeof (CoreHeap)];

#ifdef _DEBUG
	bool initialized_;
#endif
};

extern CoreHeapObj g_core_heap;

class Heap final :
	public HeapBase
{
public:
	static void initialize ()
	{
		g_core_heap.initialize ();
	}

	static void terminate ()
	{
		g_core_heap.terminate ();
	}

	Heap (size_t allocation_unit = HEAP_UNIT_DEFAULT) :
		HeapBase (allocation_unit)
	{}

	~Heap ()
	{
		cleanup ();
		// TODO: Log message if memory leaks detected.
	}

	/// \brief Releases all memory.
	/// \returns `true` if no memory leaks.
	bool cleanup ();
};

template <class T> inline
void CoreAllocator <T>::deallocate (T* p, size_t cnt)
{
	g_core_heap->release (p, cnt * sizeof (T));
}

template <class T> inline
T* CoreAllocator <T>::allocate (size_t cnt, void* hint, UWord flags)
{
	return (T*)g_core_heap->allocate (hint, cnt * sizeof (T), flags);
}

}
}

#endif
