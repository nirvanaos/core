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
#include "StreamInSM.h"
#include <Nirvana/bitutils.h>
#include "../MemContext.h"

using namespace Nirvana::Core;

namespace Nirvana {
namespace ESIOP {

StreamInSM::~StreamInSM ()
{
	// Release segments
	for (Segment* p = segments_; p;) {
		Segment* next = p->next;
		Port::Memory::release (p, p->allocated_size);
		p = next;
	}

	// Release blocks
	for (BlockHdr* p = cur_block_; p;) {
		BlockHdr* next = p->next;
		Port::Memory::release (p, p->size);
		p = next;
	}
}

void StreamInSM::next_block ()
{
	BlockHdr* block = cur_block_;
	const uint8_t* block_end = (const uint8_t*)block + block->size;
	assert (cur_ptr_ <= block_end);
	size_t align_gap = block_end - cur_ptr_;
	if (size_ - position_ < align_gap)
		throw_MARSHAL (MARSHAL_MINOR_FEWER);
	position_ += align_gap;
	cur_block_ = block->next;
	Port::Memory::release (block, block->size);
	if (!cur_block_)
		throw_MARSHAL (9);
	cur_ptr_ = (const uint8_t*)(cur_block_ + 1);
}

void StreamInSM::check_position (size_t align, size_t size) const
{
	assert (size); // must be checked before

	size_t pos = round_up (position_, align);
	if ((pos > size_) || (size_ - pos < size) || !cur_block_)
		throw_MARSHAL (MARSHAL_MINOR_FEWER);
}

void StreamInSM::set_position (size_t align, size_t size) NIRVANA_NOEXCEPT
{
	position_ = round_up (position_, align) + size;
}

const StreamInSM::Segment* StreamInSM::get_segment (size_t align, size_t size)
{
	if (!segments_)
		return nullptr;

	// Potential data begin without segment
	const uint8_t* data = round_up (cur_ptr_, align);

	// Find potential segment
	const Segment* segment = (const Segment*)round_up (cur_ptr_, alignof (Segment));
	const uint8_t* block_end = (const uint8_t*)cur_block_ + cur_block_->size;
	BlockHdr* seg_block = cur_block_;
	ptrdiff_t space_to_segment = (const uint8_t*)segment - data;
	if (block_end - (const uint8_t*)segment < sizeof (Segment)) {
		// Segment may be in the next block
		seg_block = cur_block_->next;
		if (seg_block) {
			segment = (const Segment*)((const uint8_t*)seg_block + 1);
			space_to_segment = block_end - data;
		} else
			segment = nullptr;
	}
	if (space_to_segment < 0)
		space_to_segment = 0;
	if (segment == segments_ && (size_t)space_to_segment < size) {
		// Adopt segment
		assert (segment->allocated_size >= size);
		if (segment->allocated_size < size)
			throw_MARSHAL (); // Unexpected
		segments_ = segment->next;
		if (seg_block != cur_block_)
			next_block ();
		cur_ptr_ = (const uint8_t*)(segment + 1);
		return segment;
	}
	return nullptr;
}

void StreamInSM::physical_read (size_t align, size_t size, void* buf)
{
	uint8_t* dst = (uint8_t*)buf;
	do {
		const uint8_t* src = round_up (cur_ptr_, align);
		const uint8_t* block_end = (const uint8_t*)cur_block_ + cur_block_->size;
		ptrdiff_t cb = block_end - src;

		if (cb < (ptrdiff_t)align) {
			next_block ();
			src = round_up (cur_ptr_, align);
			const uint8_t* block_end = (const uint8_t*)cur_block_ + cur_block_->size;
			ptrdiff_t cb = block_end - src;
		}

		if ((size_t)cb > size)
			cb = size;
		const uint8_t* end = src + cb;
		dst = real_copy (src, end, dst);
		cur_ptr_ = end;
		size -= cb;
		// Adjust alignment if the remaining size less than it
		if (align > size)
			align = size;
	} while (size);
}

void StreamInSM::read (size_t align, size_t size, void* buf)
{
	if (!size)
		return;
	check_position (align, size);

	const Segment* segment = get_segment (align, size);
	if (segment) {
		Port::Memory::copy (buf, segment->pointer, size, Memory::SRC_DECOMMIT);
		Port::Memory::release (segment->pointer, segment->allocated_size);
	} else
		physical_read (align, size, buf);

	set_position (align, size);
}

void* StreamInSM::read (size_t align, size_t& size)
{
	if (!size)
		return nullptr;
	check_position (align, size);

	void* ret = nullptr;
	const Segment* segment = get_segment (align, size);
	if (segment) {
		// Adopt segment
		size = segment->allocated_size;
		ret = segment->pointer;
	} else {
		// Allocate buffer and read
		size_t cb_read = size;
		ret = MemContext::current ().heap ().allocate (nullptr, size, 0);
		try {
			physical_read (align, cb_read, ret);
		} catch (...) {
			MemContext::current ().heap ().release (ret, size);
			throw;
		}
	}
	return ret;
}

void StreamInSM::set_size (size_t size)
{
	if (size < position_)
		throw_BAD_PARAM ();
	size_ = position_ + size;
}

size_t StreamInSM::end ()
{
	return size_ - position_ < 8; // 8-byte alignment ignored
}

}
}
