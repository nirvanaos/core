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
#include "Heap.h"
#include <atomic>
#include <iostream>

//#define THROW(e) { std::cerr << #e << " " << __FILE__ << ":" << __LINE__ << std::endl; throw_##e ();}
#define THROW(e) throw_##e ()

namespace Nirvana {
namespace Core {

using namespace std;

CoreHeapObj g_core_heap;

inline
bool Heap::MemoryBlock::collapse_large_block (size_t size) NIRVANA_NOEXCEPT
{
	assert (is_large_block ());
	size |= 1;
	return atomic_compare_exchange_strong ((volatile atomic <size_t>*) & large_block_size_, &size, 1);
}

inline
void Heap::MemoryBlock::restore_large_block (size_t size) NIRVANA_NOEXCEPT
{
	assert (large_block_size_ == 1);
	large_block_size_ = size | 1;
}

Heap::LBErase::LBErase (BlockList& block_list, BlockList::NodeVal* first_node) :
	block_list_ (block_list),
	new_node_ (nullptr)
{
	assert (first_node);
	first_block_.node = first_node;
	last_block_.node = nullptr;
}

Heap::LBErase::~LBErase ()
{
	if (last_block_.node)
		block_list_.release_node (last_block_.node);
	for (auto& mb : middle_blocks_) {
		block_list_.release_node (mb.node);
	}
	block_list_.release_node (first_block_.node);
	if (new_node_)
		block_list_.release_node (new_node_);
}

void Heap::LBErase::prepare (void* p, size_t size)
{
	MemoryBlock* block = &first_block_.node->value ();
	assert (block->is_large_block ());
	if (!(first_block_.size = block->large_block_size ()))
		THROW (BAD_PARAM); // Block is collapsed in another thread.

	// Collect contiguos blocks.
	const uintptr_t au = Port::Memory::ALLOCATION_UNIT;
	uint8_t* end = round_up ((uint8_t*)p + size, au);
	uint8_t* block_begin = block->begin ();
	assert (block_begin <= p);
	shrink_size_ = round_down ((uint8_t*)p, au) - block_begin; // First block shrink size
	uint8_t* block_end = block_begin + first_block_.size;
	assert (block_end > p);
	while (end > block_end) {
		if (last_block_.node)
			middle_blocks_.push_back (last_block_);
		if (!(last_block_.node = block_list_.find (block_end)))
			THROW (BAD_PARAM);
		block = &last_block_.node->value ();
		if (!block->is_large_block ())
			THROW (BAD_PARAM);
		if (!(last_block_.size = block->large_block_size ()))
			THROW (BAD_PARAM);
		block_end = block->begin () + last_block_.size;
	}

	// Create new block at end if need.
	if (end < block_end)
		new_node_ = block_list_.create_node (end, block_end - end);

	// Collapse collected blocks.
	if (last_block_.node && !last_block_.collapse ())
		THROW (BAD_PARAM);

	for (MiddleBlocks::reverse_iterator it = middle_blocks_.rbegin (); it != middle_blocks_.rend (); ++it) {
		if (!it->collapse ()) {
			if (last_block_.node)
				last_block_.restore ();
			while (it > middle_blocks_.rbegin ()) {
				(--it)->restore ();
			}
			THROW (BAD_PARAM);
		}
	}

	try {
		if (!first_block_.collapse ())
			THROW (BAD_PARAM);
	} catch (...) {
		rollback_helper ();
		throw;
	}
}

void Heap::LBErase::commit () NIRVANA_NOEXCEPT
{
	if (shrink_size_)
		first_block_.restore (shrink_size_);

	if (new_node_)
		block_list_.insert (new_node_);

	// Cleanup
	if (last_block_.node)
		block_list_.remove (last_block_.node);

	for (auto& mb : middle_blocks_) {
		block_list_.remove (mb.node);
	}

	block_list_.remove (first_block_.node);
}

void Heap::LBErase::rollback () NIRVANA_NOEXCEPT
{
	rollback_helper ();
	first_block_.restore ();
	if (last_block_.node)
		last_block_.restore ();
}

void Heap::LBErase::rollback_helper () NIRVANA_NOEXCEPT
{
	for (MiddleBlocks::iterator it = middle_blocks_.begin (); it != middle_blocks_.end (); ++it) {
		it->restore ();
	}
}

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
		THROW (FREE_MEM);
	const MemoryBlock& block = node->value ();
	if (!block.is_large_block ()) {
		// Release from heap partition
		uint8_t* part_end = block.begin () + partition_size ();
		block_list_.release_node (node);
		if ((uint8_t*)p + size <= part_end)
			release (block.directory (), p, size);
		else
			THROW (FREE_MEM);
	} else {
		// Release large block
		LBErase lberase (block_list_, node);
		lberase.prepare (p, size);
		try {
			Port::Memory::release (p, size);
		} catch (...) {
			lberase.rollback ();
			throw;
		}
		lberase.commit ();
	}
}

void Heap::release (Directory& part, void* p, size_t size) const
{
	uint8_t* heap = (uint8_t*)(&part + 1);
	size_t offset = (uint8_t*)p - heap;
	size_t begin = offset / allocation_unit_;
	size_t end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
	if (!part.check_allocated (begin, end))
		THROW (FREE_MEM);
	HeapInfo hi = { heap, allocation_unit_, Port::Memory::OPTIMAL_COMMIT_UNIT };
	part.release (begin, end, &hi);
}

Heap::Directory* Heap::get_partition (const void* p) NIRVANA_NOEXCEPT
{
	assert (p);
	Directory* part = nullptr;
	BlockList::NodeVal* node = block_list_.lower_bound (p);
	if (node) {
		const MemoryBlock& block = node->value ();
		if (block.is_large_block ())
			block_list_.release_node (node);
		else {
			block_list_.release_node (node);
			if (p < block.begin () + partition_size ())
				part = &block.directory ();
		}
	}
	return part;
}

void* Heap::allocate (void* p, size_t size, unsigned flags)
{
	if (!size)
		THROW (BAD_PARAM);

	if ((flags & ~(Memory::RESERVED | Memory::READ_ONLY | Memory::EXACTLY | Memory::ZERO_INIT))
		|| ((flags & Memory::READ_ONLY) && !(flags & Memory::RESERVED))
	)
		throw_INV_FLAG ();

	if (p) {
		// Explicit address allocation
		Directory* dir = get_partition (p);
		if (dir) {
			if ((p = allocate (*dir, p, size, flags)) || (flags & Memory::EXACTLY))
				return p;
		} else {
			if ((p = Port::Memory::allocate (p, size, flags | Memory::EXACTLY)))
				add_large_block (p, size);
			else if (flags & Memory::EXACTLY)
				return nullptr;
		}
	}

	if (!p)
		p = allocate (size, flags);

	return p;
}

void* Heap::allocate (size_t size, unsigned flags)
{
	void* p;
	const size_t max_block_size = Directory::MAX_BLOCK_SIZE * allocation_unit_;
	if (size > max_block_size
		||
		(max_block_size >= Port::Memory::ALLOCATION_UNIT && !(size % Port::Memory::ALLOCATION_UNIT))
		||
		((flags & Memory::RESERVED) && size >= Port::Memory::OPTIMAL_COMMIT_UNIT)
		) {
		if (!(p = Port::Memory::allocate (nullptr, size, flags))) {
			assert (flags & Memory::EXACTLY);
			return nullptr;
		}
		add_large_block (p, size);
	} else {
		try {
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
		} catch (const CORBA::NO_MEMORY&) {
			if (flags & Memory::EXACTLY)
				return nullptr;
			throw;
		}
	}
	return p;
}

void Heap::add_large_block (void* p, size_t size)
{
	try {
		const uintptr_t au = Port::Memory::ALLOCATION_UNIT;
		uint8_t* begin = round_down ((uint8_t*)p, au);
		uint8_t* end = round_up ((uint8_t*)p + size, au);
		block_list_.insert (begin, end - begin);
	} catch (...) {
		Port::Memory::release (p, size);
		throw;
	}
}

void* Heap::allocate (Directory& part, size_t size, unsigned flags, size_t allocation_unit) NIRVANA_NOEXCEPT
{
	size_t units = (size + allocation_unit - 1) / allocation_unit;
	uint8_t* heap = (uint8_t*)(&part + 1);
	HeapInfo hi = {heap, allocation_unit, Port::Memory::OPTIMAL_COMMIT_UNIT};
	ptrdiff_t unit = part.allocate (units, flags & Memory::RESERVED ? nullptr : &hi);
	if (unit >= 0) {
		assert (unit < Directory::UNIT_COUNT);
		void* p = heap + unit * allocation_unit;
		if (flags & Memory::ZERO_INIT)
			zero ((size_t*)p, (size_t*)p + (size + sizeof (size_t) - 1) / sizeof (size_t));
		return p;
	}
	return nullptr;
}

void* Heap::allocate (Directory& part, void* p, size_t size, unsigned flags) const NIRVANA_NOEXCEPT
{
	uint8_t* heap = (uint8_t*)(&part + 1);
	size_t offset = (uint8_t*)p - heap;
	size_t begin = offset / allocation_unit_;
	size_t end;

	if (flags & Memory::EXACTLY)
		end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
	else
		end = begin + (size + allocation_unit_ - 1) / allocation_unit_;

	HeapInfo hi = {heap, allocation_unit_, Port::Memory::OPTIMAL_COMMIT_UNIT};
	if (part.allocate (begin, end, flags & Memory::RESERVED ? nullptr : &hi)) {
		uint8_t* pbegin = heap + begin * allocation_unit_;
		if (!(flags & Memory::EXACTLY))
			p = pbegin;

		if (flags & Memory::ZERO_INIT)
			zero ((size_t*)pbegin, (size_t*)(heap + end * allocation_unit_));

		return p;
	}
	return nullptr;
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

Heap::Directory* Heap::create_partition () const
{
	size_t cb = sizeof (Directory) + partition_size (allocation_unit_);
	void* p = Port::Memory::allocate (nullptr, cb, Memory::RESERVED);
	try {
		return new (p) Directory ();
	} catch (...) {
		Port::Memory::release (p, cb);
		throw;
	}
}

Heap::MemoryBlock* HeapCore::add_new_partition (MemoryBlock*& tail)
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

void* Heap::copy (void* dst, void* src, size_t size, unsigned flags)
{
	if (!size)
		return dst;

	if (!src)
		THROW (BAD_PARAM);

	if (flags != Memory::SIMPLE_COPY
		&& (flags & ~(Memory::READ_ONLY | Memory::SRC_RELEASE | Memory::DST_ALLOCATE | Memory::EXACTLY))
		)
		throw_INV_FLAG ();

	if (dst == src) {
		if (check_owner (dst, size))
			return Port::Memory::copy (dst, src, size, flags);
		else if ((flags & Memory::READ_ONLY) || !Port::Memory::is_writable (dst, size))
			THROW (NO_PERMISSION);
		return dst;
	}

	unsigned release_flags = flags & Memory::SRC_RELEASE;
	if (release_flags == Memory::SRC_RELEASE) {
		BlockList::NodeVal* node = block_list_.lower_bound (src);
		if (!node)
			THROW (BAD_PARAM);
		const MemoryBlock& block = node->value ();
		if (block.is_large_block ()) {
			LBErase lberase (block_list_, node);
			lberase.prepare (src, size);
			BlockList::NodeVal* new_node = nullptr;
			try {
				new_node = block_list_.create_node ();
				dst = Port::Memory::copy (dst, src, size, flags);
				if (dst) {
					const uintptr_t au = Port::Memory::ALLOCATION_UNIT;
					uint8_t* begin = round_down ((uint8_t*)dst, au);
					uint8_t* end = round_up ((uint8_t*)dst + size, au);
					new_node->value () = MemoryBlock (begin, end - begin);
				}
			} catch (...) {
				if (new_node)
					block_list_.release_node (new_node);
				lberase.rollback ();
				throw;
			}
			if (dst) {
				lberase.commit ();
				block_list_.insert (new_node);
			} else {
				lberase.rollback ();
				block_list_.release_node (new_node);
			}
		} else {
			// Release from the heap partition
			uint8_t* heap = block.begin ();
			if ((uint8_t*)src + size > heap + partition_size ())
				THROW (FREE_MEM);
			size_t offset = (uint8_t*)src - heap;
			size_t begin = offset / allocation_unit_;
			size_t end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
			Directory* part = (Directory*)heap - 1;
			if (!part->check_allocated (begin, end))
				THROW (FREE_MEM);

			if (!dst) {
				// Memory::SRC_RELEASE is specified, so we can use source block as destination
				dst = Port::Memory::copy (src, src, size, (flags & ~Memory::SRC_RELEASE) | Memory::SRC_DECOMMIT);
			} else {
				uint8_t* rel_begin = round_down ((uint8_t*)src, allocation_unit_);
				uint8_t* rel_end = round_up ((uint8_t*)src + size, allocation_unit_);
				uint8_t* alloc_begin = nullptr;
				uint8_t* alloc_end = nullptr;
				bool dst_own = true;
				if (flags & Memory::DST_ALLOCATE) {
					alloc_begin = (uint8_t*)dst;
					alloc_end = alloc_begin + size;
					if (alloc_begin < (uint8_t*)src) {
						if (alloc_end > rel_begin) {
							uint8_t* tmp = rel_begin;
							rel_begin = round_up (alloc_end, allocation_unit_);
							alloc_end = tmp;
						}
					} else if (alloc_begin < rel_end) {
						uint8_t* tmp = rel_end;
						rel_end = round_down (alloc_begin, allocation_unit_);
						alloc_begin = tmp;
					}

					if (!allocate (alloc_begin, alloc_end - alloc_begin, (flags & ~Memory::READ_ONLY) | Memory::RESERVED | Memory::EXACTLY)) {
						if (flags & Memory::EXACTLY)
							return nullptr;
						dst = allocate (size, (flags & ~Memory::READ_ONLY) | Memory::RESERVED);
						alloc_begin = (uint8_t*)dst;
						alloc_end = alloc_begin + size;
						rel_begin = (uint8_t*)src;
						rel_end = rel_begin + size;
					}
				} else {
					dst_own = check_owner (dst, size);
					if (!dst_own && (flags & Memory::READ_ONLY))
						THROW (NO_PERMISSION);
				}

				try {
					if (dst_own)
						dst = Port::Memory::copy (dst, src, size, (flags & ~Memory::SRC_RELEASE) | Memory::SRC_DECOMMIT);
					else
						real_copy ((uint8_t*)src, (uint8_t*)src + size, (uint8_t*)dst);
				} catch (...) {
					Port::Memory::release (alloc_begin, alloc_end - alloc_begin);
					throw;
				}

				// Release memory
				offset = rel_begin - heap;
				begin = offset / allocation_unit_;
				end = (rel_end - heap + allocation_unit_ - 1) / allocation_unit_;
				part->release (begin, end, nullptr); // Source memory was already decommitted so we don't pass HeapInfo.
			}
		}

		return dst;

	} else if (release_flags) {
		assert (release_flags == Memory::SRC_DECOMMIT);
		if (!check_owner (src, size))
			THROW (NO_PERMISSION);
	}

	uint8_t* alloc_begin = nullptr;
	uint8_t* alloc_end = nullptr;
	bool dst_own = true;
	if (!dst) {
		if (flags & Memory::SIMPLE_COPY)
			throw_INV_FLAG ();

		if (!(dst = allocate (size, (flags & ~Memory::READ_ONLY) | Memory::RESERVED))) {
			assert (flags & Memory::EXACTLY);
			return nullptr;
		}
		alloc_begin = (uint8_t*)dst;
		alloc_end = alloc_begin + size;
	} else if (flags & Memory::DST_ALLOCATE) {
		uintptr_t au = query (dst, Memory::QueryParam::ALLOCATION_UNIT);
		alloc_begin = round_down ((uint8_t*)dst, au);
		alloc_end = round_up ((uint8_t*)dst + size, au);
		if (alloc_begin < (uint8_t*)src) {
			if (alloc_end > (uint8_t*)src)
				alloc_end = round_down ((uint8_t*)src, au);
		} else {
			uint8_t* src_end = (uint8_t*)src + size;
			if (alloc_begin < src_end)
				alloc_begin = round_up (src_end, au);
		}
		if (!allocate (alloc_begin, alloc_end - alloc_begin, (flags & ~Memory::READ_ONLY) | Memory::RESERVED | Memory::EXACTLY)) {
			if (flags & Memory::EXACTLY)
				return nullptr;
			dst = allocate (size, flags | Memory::RESERVED);
			alloc_begin = (uint8_t*)dst;
			alloc_end = alloc_begin + size;
		}
	} else {
		dst_own = check_owner (dst, size);
		if (!dst_own && (flags & Memory::READ_ONLY))
			throw_BAD_PARAM ();
	}

	try {
		if (dst_own)
			dst = Port::Memory::copy (dst, src, size, flags & ~Memory::DST_ALLOCATE);
		else
			real_copy ((uint8_t*)src, (uint8_t*)src + size, (uint8_t*)dst);
	} catch (...) {
		Port::Memory::release (alloc_begin, alloc_end - alloc_begin);
		throw;
	}

	return dst;
}

uintptr_t Heap::query (const void* p, Memory::QueryParam param)
{
	if (Memory::QueryParam::ALLOCATION_UNIT == param) {
		if (!p || get_partition (p))
			return allocation_unit_;
	} else if (p && (param == Memory::QueryParam::ALLOCATION_SPACE_BEGIN || param == Memory::QueryParam::ALLOCATION_SPACE_END)) {
		const Directory* part = get_partition (p);
		if (part) {
			if (param == Memory::QueryParam::ALLOCATION_SPACE_BEGIN)
				return (uintptr_t)(part + 1);
			else
				return (uintptr_t)(part + 1 + partition_size ());
		}
	} else if (Memory::QueryParam::MEMORY_STATE == param) {
		if (!check_owner (p, allocation_unit_))
			return (uintptr_t)Memory::MemoryState::MEM_NOT_ALLOCATED;
	}
	return Port::Memory::query (p, param);
}

bool Heap::check_owner (const void* p, size_t size)
{
	bool ok = false;
	BlockList::NodeVal* node = block_list_.lower_bound (p);
	if (node) {
		const MemoryBlock* block = &node->value ();
		if (block->is_large_block ()) {
			uint8_t* block_end = block->begin () + block->large_block_size ();
			if ((uint8_t*)p < block_end) {
				uint8_t* end = (uint8_t*)p + size;
				for (;;) {
					if (end <= block_end) {
						ok = true;
						break;
					}
					block_list_.release_node (node);
					node = block_list_.find (block_end);
					if (!node)
						break;
					block = &node->value ();
					if (!block->is_large_block ())
						break;
					size_t block_size = block->large_block_size ();
					if (!block_size)
						break;
					block_end = block->begin () + block_size;
				}
			}
			if (node)
				block_list_.release_node (node);
		} else {
			uint8_t* heap = block->begin ();
			if ((uint8_t*)p + size <= heap + partition_size ()) {
				size_t offset = (uint8_t*)p - heap;
				size_t begin = offset / allocation_unit_;
				size_t end = (offset + size + allocation_unit_ - 1) / allocation_unit_;
				Directory* part = (Directory*)heap - 1;
				ok = part->check_allocated (begin, end);
			}
			block_list_.release_node (node);
		}
	}
	return ok;
}

void Heap::change_protection (bool read_only)
{
	unsigned short protection = read_only ? (Memory::READ_ONLY | Memory::EXACTLY) : (Memory::READ_WRITE | Memory::EXACTLY);
	const BlockList::NodeVal* p = block_list_.head ();
	for (;;) {
		p = static_cast <BlockList::NodeVal*> (static_cast <BlockList::Node*> (p->next [0].load ()));
		if (p == block_list_.tail ())
			break;
		const MemoryBlock& mb = p->value ();
		uint8_t* begin = mb.begin ();
		uint8_t* end = begin + (mb.is_large_block () ? mb.large_block_size () : partition_size ());
		size_t commit_unit = Port::Memory::query (begin, Memory::QueryParam::COMMIT_UNIT);

		do {
			if (Port::Memory::query (begin, Memory::QueryParam::MEMORY_STATE) < (uintptr_t)Memory::MemoryState::MEM_READ_ONLY)
				begin += commit_unit;
			else {
				uint8_t* commit_end = begin + commit_unit;
				while (commit_end < end) {
					if (Port::Memory::query (commit_end, Memory::QueryParam::MEMORY_STATE) < (uintptr_t)Memory::MemoryState::MEM_READ_ONLY)
						break;
					commit_end += commit_unit;
				}
				Port::Memory::copy (begin, begin, commit_end - begin, protection);
				begin = commit_end;
			}
		} while (begin < end);
	}
}

bool HeapUser::cleanup ()
{
	bool empty = true;
	part_list_ = nullptr;
	while (BlockList::NodeVal* node = block_list_.delete_min ()) {
		const MemoryBlock block = node->value ();
		block_list_.release_node (node);
		if (block.is_large_block ()) {
			Port::Memory::release (block.begin (), block.large_block_size ());
			empty = false;
		} else {
			Directory& dir = block.directory ();
			if (!dir.empty ())
				empty = false;
			Port::Memory::release (&dir, sizeof (Directory) + partition_size ());
		}
	}
	return empty;
}

}
}
