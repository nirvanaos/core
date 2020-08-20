#include "core.h"
#include <CORBA/CORBA.h>
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace std;

Heap::Heap (size_t allocation_unit) NIRVANA_NOEXCEPT :
	allocation_unit_ (allocation_unit),
	part_list_ (nullptr)
{
	if (allocation_unit <= HEAP_UNIT_MIN)
		allocation_unit_ = HEAP_UNIT_MIN;
	else if (allocation_unit >= HEAP_UNIT_MAX)
		allocation_unit_ = HEAP_UNIT_MAX;
	else
		allocation_unit_ = clp2 (allocation_unit);
}

void Heap::release (void* p, size_t size)
{
	if (!p || !size)
		return;

	// Search memory block
	BlockList::NodeVal* node = block_list_.lower_bound (p);
	if (!node)
		throw CORBA::BAD_PARAM ();
	const MemoryBlock& block = node->value ();
	if (!block.is_large_block ()) {
		// Release from heap partition
		uint8_t* part_end = block.begin () + partition_size ();
		block_list_.release_node (node);
		if ((uint8_t*)p + size <= part_end)
			release (block.directory (), p, size);
		else
			throw CORBA::BAD_PARAM ();
	} else {
		// Release large block
		uint8_t* block_begin = block.begin ();
		uint8_t* block_end = block_begin + block.large_block_size ();
		uint8_t* release_begin = (uint8_t*)p;
		uint8_t* release_end = (uint8_t*)p + size;
		if (release_begin > block_begin || release_end < block_end) {
			size_t au = (size_t)Port::ProtDomainMemory::query (block_begin, MemQuery::ALLOCATION_UNIT);
			release_begin = round_down (release_begin, au);
			release_end = round_up (release_end, au);
		}
		// Memory may be released partially, so it may be up to 2 new nodes.
		BlockList::NodeVal* new_node0 = nullptr, * new_node1 = nullptr;
		try {
			if (release_end <= block_end) {
				
				// Release memory from single large block
				if (block_begin < release_begin)
					new_node0 = block_list_.create_node (block_begin, release_begin - block_begin);
				if (release_end < block_end)
					new_node1 = block_list_.create_node (release_end, block_end - release_end);
				if (!block_list_.remove (node))
					throw CORBA::BAD_PARAM (); // Concurrently removed in another thread.
				block_list_.release_node (node);

				// Add new blocks and release protection domain memory.
				// This mustn't cause any exceptions.
				if (new_node0)
					block_list_.insert (new_node0);
				if (new_node1)
					block_list_.insert (new_node1);
				Port::ProtDomainMemory::release (release_begin, release_end - release_begin);

			} else {
				// Release memory across multiple large blocks. Rare case.
				// Collect pointers to all large block nodes in list to ensure that we own all of this blocks.
				vector <BlockList::NodeVal*> middle_blocks;
				BlockList::NodeVal* last_node = nullptr;
				try {
					const MemoryBlock* last_block;
					for (;;) {
						last_node = block_list_.find (block_end);
						if (!last_node)
							throw CORBA::BAD_PARAM ();
						last_block = &last_node->value ();
						if (!last_block->is_large_block ())
							throw CORBA::BAD_PARAM ();
						block_end = last_block->begin () + last_block->large_block_size ();
						if (release_end <= block_end)
							break;
						middle_blocks.push_back (last_node);
					}

					// Collected.
					if (release_end < block_end) {
						size_t au = (size_t)Port::ProtDomainMemory::query (last_block->begin (), MemQuery::ALLOCATION_UNIT);
						release_end = round_up (release_end, au);
					}
					if (block_begin < release_begin)
						new_node0 = block_list_.create_node (block_begin, release_begin - block_begin);
					if (release_end < block_end)
						new_node1 = block_list_.create_node (release_end, block_end - release_end);

					// Release block-by-block.
					// If block_list_.remove () returns false for some block, just skip it.
					// This may be caused by the collision and we violate rules.
					// We can not implement atomic memory release for this case.
					// So if we threw an exception, we could leave memory in an unpredictable state.

					// Release first block
					bool ok = block_list_.remove (node);
					assert (ok); // Assert can be caused only by a wrong memory release pointer in other thread.
					block_list_.release_node (node);
					node = nullptr;
					if (ok) {
						if (new_node0)
							block_list_.insert (new_node0);
						Port::ProtDomainMemory::release (release_begin, block_begin + block.large_block_size () - release_begin);
					} else if (new_node0) {
						block_list_.release_node (new_node0);
						new_node0 = nullptr;
					}

					// Release middle blocks
					for (size_t i = 0; i < middle_blocks.size (); ++i) {
						BlockList::NodeVal*& p = middle_blocks [i];
						ok = block_list_.remove (p);
						assert (ok); // Assert can be caused only by a wrong memory release pointer in other thread.
						if (ok) {
							const MemoryBlock& block = p->value ();
							Port::ProtDomainMemory::release (block.begin (), block.large_block_size ());
						}
						block_list_.release_node (p);
						p = nullptr;
					}

					// Release last block
					ok = block_list_.remove (last_node);
					assert (ok); // Assert can be caused only by a wrong memory release pointer in other thread.
					block_list_.release_node (last_node);
					last_node = nullptr;
					if (ok) {
						if (new_node1)
							block_list_.insert (new_node1);
						block_begin = last_block->begin ();
						Port::ProtDomainMemory::release (block_begin, release_end - block_begin);
					} else if (new_node1) {
						block_list_.release_node (new_node1);
						new_node1 = nullptr;
					}

				} catch (...) {
					if (last_node)
						block_list_.release_node (last_node);
					while (!middle_blocks.empty ()) {
						BlockList::NodeVal* p = middle_blocks.back ();
						if (!p)
							break;
						block_list_.release_node (p);
						middle_blocks.pop_back ();
					}
					throw;
				}
			}
		} catch (...) {
			block_list_.release_node (node);
			if (new_node0)
				block_list_.release_node (new_node0);
			if (new_node1)
				block_list_.release_node (new_node1);
			throw;
		}
	}
}

void Heap::release (Directory& part, void* p, size_t size) const
{
	uint8_t* heap = (uint8_t*)(&part + 1);
	UWord offset = (uint8_t*)p - heap;
	UWord begin = offset / allocation_unit_;
	UWord end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
	if (!part.check_allocated (begin, end))
		throw CORBA::BAD_PARAM ();
	HeapInfo hi = { heap, allocation_unit_, Port::ProtDomainMemory::OPTIMAL_COMMIT_UNIT };
	part.release (begin, end, &hi);
}

Heap::Directory* Heap::get_partition (const void* p) NIRVANA_NOEXCEPT
{
	assert (p);
	Directory* part = nullptr;
	BlockList::NodeVal* node = block_list_.lower_bound (p);
	if (node) {
		const MemoryBlock& block = node->value ();
		if (!block.is_large_block () && p <= block.begin () + partition_size ())
			part = &block.directory ();
		block_list_.release_node (node);
	}
	return part;
}

void* Heap::allocate (void* p, size_t size, UWord flags)
{
	if (!size)
		throw CORBA::BAD_PARAM ();

	if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
		throw CORBA::INV_FLAG ();

	if (p) {
		Directory* part = get_partition (p);
		if (part) {
			if (!allocate (*part, p, size, flags))
				if (flags & Memory::EXACTLY)
					return nullptr;
				else
					p = nullptr;
		} else {
			p = Port::ProtDomainMemory::allocate (p, size, flags | Memory::EXACTLY);
			if (!p) {
				if (flags & Memory::EXACTLY)
					return nullptr;
			} else {
				try {
					add_large_block (p, size);
				} catch (...) {
					Port::ProtDomainMemory::release (p, size);
					throw;
				}
			}
		}
	}

	if (!p) {
		if (size > Directory::MAX_BLOCK_SIZE * allocation_unit_ || ((flags & Memory::RESERVED) && size >= Port::ProtDomainMemory::OPTIMAL_COMMIT_UNIT)) {
			p = Port::ProtDomainMemory::allocate (nullptr, size, flags);
			try {
				add_large_block (p, size);
			} catch (...) {
				Port::ProtDomainMemory::release (p, size);
				throw;
			}
		} else {
			try {
				p = allocate (size, flags);
			} catch (const CORBA::NO_MEMORY&) {
				if (flags & Memory::EXACTLY)
					return nullptr;
				throw;
			}
			if (flags & Memory::ZERO_INIT)
				zero ((Word*)p, (Word*)p + (size + sizeof (Word) - 1) / sizeof (Word));
		}
	}

	return p;
}

void* Heap::allocate (size_t size, UWord flags)
{
	MemoryBlock** link = &part_list_;
	for (;;) {
		MemoryBlock* part = *link;
		if (part) {
			void* p = allocate (part->directory (), size, flags);
			if (p)
				return p;
			link = &part->next_partition ();
		} else
			part = add_new_partition (*link);
	}
}

void Heap::add_large_block (void* p, size_t size)
{
	try {
		size_t au = (size_t)Port::ProtDomainMemory::query (p, MemQuery::ALLOCATION_UNIT);
		uint8_t* begin = round_down ((uint8_t*)p, au);
		uint8_t* end = round_up ((uint8_t*)p + size, au);
		block_list_.insert (begin, end - begin);
	} catch (...) {
		Port::ProtDomainMemory::release (p, size);
		throw;
	}
}

void* Heap::allocate (Directory& part, size_t size, UWord flags, size_t allocation_unit) NIRVANA_NOEXCEPT
{
	size_t units = (size + allocation_unit - 1) / allocation_unit;
	uint8_t* heap = (uint8_t*)(&part + 1);
	HeapInfo hi = {heap, allocation_unit, Port::ProtDomainMemory::OPTIMAL_COMMIT_UNIT};
	ptrdiff_t unit = part.allocate (units, flags & Memory::RESERVED ? nullptr : &hi);
	if (unit >= 0) {
		assert (unit < Directory::UNIT_COUNT);
		return heap + unit * allocation_unit;
	}
	return 0;
}

bool Heap::allocate (Directory& part, void* p, size_t size, UWord flags) const NIRVANA_NOEXCEPT
{
	uint8_t* heap = (uint8_t*)(&part + 1);
	size_t offset = (uint8_t*)p - heap;
	size_t begin = offset / allocation_unit_;
	size_t end;

	if (flags & Memory::EXACTLY)
		end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
	else
		end = begin + (size + allocation_unit_ - 1) / allocation_unit_;

	HeapInfo hi = {heap, allocation_unit_, Port::ProtDomainMemory::OPTIMAL_COMMIT_UNIT};
	if (part.allocate (begin, end, flags & Memory::RESERVED ? nullptr : &hi)) {
		uint8_t* pbegin = heap + begin * allocation_unit_;
		if (!(flags & Memory::EXACTLY))
			p = pbegin;

		if (flags & Memory::ZERO_INIT)
			zero ((Word*)pbegin, (Word*)(heap + end * allocation_unit_));

		return true;
	}
	return false;
}

Heap::MemoryBlock* Heap::add_new_partition (MemoryBlock*& tail)
{
	Directory* dir = create_partition ();
	BlockList::NodeVal* node;
	try {
		node = block_list_.insert (dir);
	} catch (...) {
		release_partition (dir);
		throw;
	}
	MemoryBlock* part = &node->value ();
	MemoryBlock* next = nullptr;
	if (!atomic_compare_exchange_strong ((volatile atomic <MemoryBlock*>*)&tail, &next, part)) {
		// New partition was already added in another thread.
		part = next;
		block_list_.remove (node);
		release_partition (dir);
	}
	block_list_.release_node (node);
	return part;
}

Heap::MemoryBlock* CoreHeap::add_new_partition (MemoryBlock*& tail)
{
	// Core heap has different algorithm to avoid recursion.
	// The block list node is allocated from the new partition.
	Directory* dir = create_partition ();
	BlockList::NodeVal* node = block_list_.insert (dir, allocation_unit_);
	MemoryBlock* part = &node->value ();
	MemoryBlock** link = &tail;
	MemoryBlock* next = nullptr;
	MemoryBlock* ret = part;
	while (!atomic_compare_exchange_weak ((volatile atomic <MemoryBlock*>*)link, &next, part)) {
		// New partition was already added in another thread.
		if (ret == part)
			ret = next; // Return the first new partition added.
		// We can not remove the node from list and release partition here.
		// In the skip list algorithm we can not predict when the node will be really destroyed.
		// So we can not release the partition right now.
		// We repeat the linking.
		link = &next->next_partition ();
		next = nullptr;
	}
	block_list_.release_node (node);
	return ret;
}

void* Heap::copy (void* dst, void* src, size_t size, UWord flags)
{
	if (!size)
		return dst;

	if (src != dst && !(flags & Memory::READ_ONLY)) {

		if (!src)
			throw CORBA::BAD_PARAM ();

		if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
			throw CORBA::INV_FLAG ();

		Directory* release_part = nullptr;
		size_t release_size;
		void* release_ptr = nullptr;
		if (flags & Memory::RELEASE) {
			release_part = get_partition (src);
			release_ptr = src;
			release_size = size;
		}

		if (dst && (flags & Memory::ALLOCATE)) {
			Directory* part = get_partition (dst);
			if (part) {
				void* alloc_ptr = dst;
				size_t alloc_size = size;
				if (dst < src) {
					uint8_t* dst_end = (uint8_t*)dst + size;
					if (dst_end > src) {
						alloc_size = (uint8_t*)src - (uint8_t*)dst;
						if (flags & Memory::RELEASE) {
							release_ptr = dst_end;
							release_size = alloc_size;
						}
					}
				} else {
					uint8_t* src_end = (uint8_t*)src + size;
					if (src_end > dst) {
						alloc_ptr = src_end;
						alloc_size = (uint8_t*)dst - (uint8_t*)src;
						if (flags & Memory::RELEASE)
							release_size = alloc_size;
					}
				}
				if (allocate (*part, alloc_ptr, alloc_size, flags))
					flags &= ~Memory::ALLOCATE;
				else if (flags & Memory::EXACTLY)
					return 0;
				else {
					dst = 0;
					if (flags & Memory::RELEASE) {
						release_ptr = src;
						release_size = size;
					}
				}
			}
		}

		if (!dst && size < Port::ProtDomainMemory::ALLOCATION_UNIT / 2) {
			assert (!(flags & ~(Memory::ALLOCATE | Memory::RELEASE | Memory::EXACTLY)));
			try {
				dst = allocate (size, flags);
			} catch (const CORBA::NO_MEMORY&) {
				if (flags & Memory::EXACTLY)
					return nullptr;
				throw;
			}
			flags &= ~Memory::ALLOCATE;
		}

		void* ret;
		if (release_part) {
			ret = Port::ProtDomainMemory::copy (dst, src, size, flags & ~Memory::RELEASE);
			release (*release_part, release_ptr, release_size);
		} else
			ret = Port::ProtDomainMemory::copy (dst, src, size, flags);

		return ret;
	}

	return Port::ProtDomainMemory::copy (dst, src, size, flags);
}

uintptr_t Heap::query (const void* p, MemQuery param)
{
	if (MemQuery::ALLOCATION_UNIT == param) {
		if (!p || get_partition (p))
			return allocation_unit_;
	} else if (p && (param == MemQuery::ALLOCATION_SPACE_BEGIN || param == MemQuery::ALLOCATION_SPACE_END)) {
		const Directory* part = get_partition (p);
		if (part) {
			if (param == MemQuery::ALLOCATION_SPACE_BEGIN)
				return (uintptr_t)(part + 1);
			else
				return (uintptr_t)(part + 1 + partition_size ());
		}
	}
	return Port::ProtDomainMemory::query (p, param);
}

void Heap::initialize ()
{
	Port::ProtDomainMemory::initialize ();
	Directory* part = create_partition (HEAP_UNIT_CORE);
	Heap* heap = new (allocate (*part, sizeof (CoreHeap), 0, HEAP_UNIT_CORE)) CoreHeap ();
	BlockList::NodeVal* node = heap->block_list_.insert (part, HEAP_UNIT_CORE);
	heap->part_list_ = &node->value ();
	heap->block_list_.release_node (node);
	g_core_heap = heap;
}

void Heap::terminate ()
{
	Port::ProtDomainMemory::terminate ();
}

}
}
