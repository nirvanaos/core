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
#include "../pch.h"
#include "StreamOutSM.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

void StreamOutSM::initialize ()
{
	size_ = 0;
	commit_unit_ = 0;
	chunk_ = nullptr;
	other_domain_->get_sizes (sizes_);
	allocate_block (sizes_.sizeof_pointer, sizes_.sizeof_pointer);
	stream_hdr_ = cur_block ().other_ptr;
	segments_tail_ = cur_ptr_;
	uint8_t* ptr = (uint8_t*)other_domain_->store_pointer (segments_tail_, 0); // segments
	cur_ptr_ = round_up (ptr, 8);
}

void StreamOutSM::clear (size_t leave_header) noexcept
{
	try {
		while (!other_allocated_.empty ()) {
			const auto& a = other_allocated_.back ();
			other_domain_->release (a.ptr, a.size);
			other_allocated_.pop_back ();
		}
		while (blocks_.size () > leave_header) {
			const auto& a = blocks_.back ();
			if (a.other_ptr)
				other_domain_->release (a.other_ptr, round_up (a.size, sizes_.block_size));
			if (a.ptr)
				Nirvana::Core::Port::Memory::release (a.ptr, a.size);
			blocks_.pop_back ();
		}
	} catch (...) {
		other_allocated_.clear ();
		blocks_.resize (leave_header);
	}
}

void StreamOutSM::write (size_t align, size_t element_size, size_t CDR_element_size,
	size_t element_count, void* data, size_t& allocated_size)
{
	size_t size = element_size * element_count;
	if (!size)
		return;

	if (!data)
		throw_BAD_PARAM ();

	if (sizes_.max_size - size_ < size)
		throw_IMP_LIMIT ();
	size_t new_size = round_up (size_, align);
	if (sizes_.max_size - new_size < size)
		throw_IMP_LIMIT ();
	new_size += size;

	Block block = cur_block ();
	uint8_t* block_end = (uint8_t*)block.ptr + block.size;

	if (size > sizes_.allocation_unit / 2) {
		// Virtual copy

		other_allocated_.emplace_back ();
		OtherAllocated& oa = other_allocated_.back ();
		size_t adopted_size = size;
		oa.ptr = other_domain_->copy (0, data, adopted_size, (allocated_size != 0) ? Nirvana::Memory::SRC_DECOMMIT : 0);
		oa.size = adopted_size;
		if (allocated_size) {
			Nirvana::Core::Heap::user_heap ().release ((uint8_t*)data, allocated_size);
			allocated_size = 0;
		}

		// Reserve space for StreamInSM::Segment
		ptrdiff_t cb_segment = 2 * sizes_.sizeof_pointer + sizes_.sizeof_size;
		uint8_t* p = round_up (cur_ptr_, sizes_.sizeof_pointer);
		if (cb_segment > block_end - p) {
			// We need a new block
			allocate_block (sizes_.sizeof_pointer, cb_segment);
			block = cur_block ();
			p = cur_ptr_;
		}

		// Store segment to stream
		size_t offset = p - (uint8_t*)block.ptr;
		other_domain_->store_pointer (segments_tail_, block.other_ptr + offset);
		segments_tail_ = p;
		if (commit_unit_) {
			size_t segment_size = sizes_.sizeof_pointer * 2 + sizes_.sizeof_size;
			if ((offset + segment_size) / commit_unit_ != offset / commit_unit_)
				Port::Memory::commit (p, segment_size);
		}
		p = (uint8_t*)other_domain_->store_pointer (p, 0); // next
		p = (uint8_t*)other_domain_->store_pointer (p, oa.ptr); // address
		p = (uint8_t*)other_domain_->store_size (p, size);
		cur_ptr_ = p;

		purge ();
	} else {
		// Physical copy

		const uint8_t* src = (const uint8_t*)data;
		do {
			uint8_t* dst = round_up (cur_ptr_, align);
			ptrdiff_t cb = block_end - dst;
			if (cb < (ptrdiff_t)align) {
				allocate_block (align, size);
				block = cur_block ();
				block_end = (uint8_t*)block.ptr + block.size;
				dst = round_up (cur_ptr_, align);
				cb = block_end - dst;
				if ((size_t)cb > size)
					cb = size;
			} else if (Port::Memory::FLAGS & Nirvana::Memory::SPACE_RESERVATION) {
				assert (commit_unit_);
				if ((size_t)cb > size)
					cb = size;
				if (((uintptr_t)dst + cb) / commit_unit_ != (uintptr_t)(cur_ptr_ - 1) / commit_unit_)
					Port::Memory::commit (dst, cb);
			}

			real_copy (src, src + cb, dst);
			cur_ptr_ = dst + cb;
			src += cb;
			size -= cb;
			// Adjust alignment if the remaining size less than it
			if (align > size)
				align = flp2 (size);
		} while (size);
	}

	size_ = new_size;
}

void StreamOutSM::allocate_block (size_t align, size_t size)
{
	size_t hdr_size = sizes_.sizeof_pointer + sizes_.sizeof_size;
	size_t data_offset = round_up (hdr_size, align);
	size_t cb = round_up (data_offset + size, sizes_.block_size);

	blocks_.emplace_back ();
	Block& block = blocks_.back ();
	if (Port::Memory::FLAGS & Nirvana::Memory::SPACE_RESERVATION) {
		block.ptr = Port::Memory::allocate (nullptr, cb, Nirvana::Memory::RESERVED);
		if (Port::Memory::FIXED_COMMIT_UNIT)
			commit_unit_ = Port::Memory::FIXED_COMMIT_UNIT;
		else
			commit_unit_ = (size_t)Port::Memory::query (block.ptr, Nirvana::Memory::QueryParam::COMMIT_UNIT);
		Port::Memory::commit (block.ptr, size);
	} else
		block.ptr = Port::Memory::allocate (nullptr, cb, 0);
	block.size = cb;
	block.other_ptr = other_domain_->reserve (cb);
	void* p = other_domain_->store_pointer (block.ptr, 0); // next = nullptr;
	other_domain_->store_size (p, cb); // size

	cur_ptr_ = (uint8_t*)block.ptr + data_offset;
	// Link to prev block
	if (blocks_.size () > 1) {
		auto prev = blocks_.begin () + blocks_.size () - 2;
		other_domain_->store_pointer (prev->ptr, block.other_ptr);
		purge ();
	}
}

// Release local memory
void StreamOutSM::purge ()
{
	if (blocks_.size () > 2) { // Never purge the first block with stream header
		for (auto it = blocks_.begin () + blocks_.size () - 2;;) {
			// If prev block does not contain the segment tail or chunk start, purge it
			void* ptr = it->ptr;
			if (ptr) { // Is not purged
				const void* end = (uint8_t*)ptr + it->size;
				if (!(ptr < segments_tail_ && segments_tail_ < end) && !(ptr < chunk_ && chunk_ < end)) {
					other_domain_->copy (it->other_ptr, it->ptr, it->size, Nirvana::Memory::SRC_RELEASE);
					it->ptr = nullptr;
				}
			}
			if (blocks_.begin () == --it)
				break;
		}
	}
}

size_t StreamOutSM::size () const
{
	return size_;
}

size_t StreamOutSM::stream_hdr_size () const noexcept
{
	return round_up (
		sizes_.sizeof_pointer // BlockHdr* next;
		+ sizes_.sizeof_size, // size_t size;
		sizes_.sizeof_pointer) + sizes_.sizeof_pointer; // Segment* segments;
}

void* StreamOutSM::header (size_t hdr_size)
{
	assert (!blocks_.empty ());
	assert (blocks_.front ().ptr);

	return (uint8_t*)blocks_.front ().ptr + stream_hdr_size ();
}

void StreamOutSM::rewind (size_t hdr_size)
{
	clear (1);
	cur_ptr_ = (uint8_t*)blocks_.front ().ptr + stream_hdr_size () + hdr_size;
	segments_tail_ = cur_ptr_ - sizes_.sizeof_pointer;
	chunk_ = nullptr;
}

void StreamOutSM::chunk_begin ()
{
	if (!chunk_) {
		Block block = cur_block ();
		uint8_t* block_end = (uint8_t*)block.ptr + block.size;
		uint8_t* p = round_up (cur_ptr_, 4);
		if (4 > block_end - p) {
			// We need a new block
			allocate_block (4, 4);
			block = cur_block ();
			p = cur_ptr_;
		}

		chunk_ = (int32_t*)(p += 4);
		cur_ptr_ = p;
		size_ += 4;
		chunk_begin_ = size_;
	}
}

CORBA::Long StreamOutSM::chunk_size () const
{
	if (chunk_) {
		size_t cb = size_ - chunk_begin_;
		if (cb >= 0x7FFFFF00)
			throw_MARSHAL ();
		return (CORBA::Long)cb;
	} else
		return -1;
}

bool StreamOutSM::chunk_end ()
{
	int32_t cs = StreamOutSM::chunk_size ();
	assert (cs != 0);
	if (cs > 0) {
		*chunk_ = cs;
		chunk_ = nullptr;
		purge ();
		return true;
	} else
		return false;
}

void StreamOutSM::store_stream (ESIOP::SharedMemPtr& where)
{
	// Truncate size of the last block
	Block& last_block = blocks_.back ();
	uint8_t* last_block_begin = (uint8_t*)last_block.ptr;
	size_t last_block_size = cur_ptr_ - last_block_begin;
	other_domain_->store_size (last_block_begin + sizes_.sizeof_pointer, last_block_size);
	last_block.size = last_block_size;

	// Purge all blocks
	for (auto it = blocks_.begin (); it != blocks_.end (); ++it) {
		if (it->ptr) {
			other_domain_->copy (it->other_ptr, it->ptr, it->size, Nirvana::Memory::SRC_RELEASE);
			it->ptr = nullptr;
		}
	}
	other_domain_->store_pointer (&where, stream_hdr_);
}

}
}
