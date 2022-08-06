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
#include "StreamOutSM.h"

using namespace Nirvana::Core;
using namespace std;

namespace Nirvana {
namespace ESIOP {

void StreamOutSM::write (size_t align, size_t size, void* data, size_t& allocated_size)
{
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

	if ((uintptr_t)data % sizes_.block_size == 0 && size >= sizes_.block_size / 2) {
		// Virtual copy

		other_allocated_.emplace_back ();
		OtherAllocated& oa = other_allocated_.back ();
		oa.ptr = other_domain_->copy (0, data, size, allocated_size != 0);
		oa.size = size;
		if (allocated_size) {
			size_t cb_release = allocated_size - size;
			allocated_size = 0;
			if (cb_release)
				MemContext::current ().heap ().release ((uint8_t*)data + size, cb_release);
		}

		// Reserve space for StreeamInSM::Segment
		ptrdiff_t cb_segment = 2 * sizes_.sizeof_pointer + sizes_.sizeof_size;
		uint8_t* p = round_up (cur_ptr_, sizes_.sizeof_pointer);
		if (cb_segment > block_end - p) {
			// We need a new block
			allocate_block (sizes_.sizeof_pointer, cb_segment);
			block = cur_block ();
			p = cur_ptr_;
		}

		// Store segment to stream
		size_t offset = (uint8_t*)block.ptr - p;
		other_domain_->store_pointer (segments_tail_, block.other_ptr + offset);
		segments_tail_ = p;
		p = (uint8_t*)other_domain_->store_pointer (p, 0); // next
		p = (uint8_t*)other_domain_->store_pointer (p, oa.ptr); // address
		p = (uint8_t*)other_domain_->store_size (p, oa.size);
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
				dst = cur_ptr_;
				cb = block_end - dst;
			}
			if ((size_t)cb > size)
				cb = size;
			const uint8_t* end = src + cb;
			dst = real_copy (src, end, dst);
			src = end;
			size -= cb;
			// Adjust alignment if the remaining size less than it
			if (align > size)
				align = size;
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
			// If prev block does not contain the segment tail, purge it
			if (it->ptr && !(it->ptr < segments_tail_ && segments_tail_ < (uint8_t*)it->ptr + it->size)) {
				other_domain_->copy (it->other_ptr, it->ptr, it->size, true);
				it->ptr = nullptr;
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

void* StreamOutSM::header (size_t hdr_size)
{
	assert (!blocks_.empty ());
	assert (blocks_.front ().ptr);

	size_t stream_hdr_size = round_up (sizes_.sizeof_pointer + sizes_.sizeof_size, sizes_.sizeof_pointer) + sizes_.sizeof_pointer;
	return (uint8_t*)blocks_.front ().ptr + stream_hdr_size;
}

}
}
