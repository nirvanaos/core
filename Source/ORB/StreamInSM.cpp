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
#include "../ExecDomain.h"
#include <limits>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace ESIOP {

inline
void StreamInSM::release_block (BlockHdr* block)
{
	size_t size = block->size;

	// The blocks allocated size is always aligned to max (ALLOCATION_UNIT, SHARING_ASSOCIATIVITY).
	// Real size of the last block may be less.
	if (Port::Memory::SHARING_ASSOCIATIVITY > Port::Memory::ALLOCATION_UNIT)
		size = round_up (size, Port::Memory::SHARING_ASSOCIATIVITY);
	Port::Memory::release (block, size);
}

StreamInSM::~StreamInSM ()
{
	// Release segments
	for (Segment* p = segments_; p;) {
		Segment* next = p->next;
		Port::Memory::release (p->pointer, p->size);
		p = next;
	}

	// Release blocks
	for (BlockHdr* p = cur_block_; p;) {
		BlockHdr* next = p->next;
		release_block (p);
		p = next;
	}
}

void StreamInSM::next_block ()
{
	BlockHdr* block = cur_block_;
	assert (cur_ptr_ <= (const uint8_t*)block + block->size);
	cur_block_ = block->next;
	release_block (block);
	if (!cur_block_)
		throw_MARSHAL (9);
	cur_ptr_ = (const uint8_t*)(cur_block_ + 1);
}

const StreamInSM::Segment* StreamInSM::get_segment (size_t align, size_t size)
{
	if (chunk_mode_) {
		if (chunk_end_ <= position_) {
			chunk_mode_ = false;
			int32_t chunk_size = read32 ();
			chunk_mode_ = true;
			if (chunk_size <= 0 || chunk_size >= 0x7FFFFF00)
				throw_MARSHAL ();
			chunk_end_ = position_ + chunk_size;
		}
	}

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
		if (segment->size != size)
			throw_MARSHAL (); // Unexpected
		segments_ = segment->next;
		if (seg_block != cur_block_)
			next_block ();
		cur_ptr_ = (const uint8_t*)(segment + 1);
		return segment;
	}
	return nullptr;
}

void StreamInSM::inc_position (size_t cb)
{
	position_ += cb;
	if (chunk_mode_ && position_ > chunk_end_)
		throw_MARSHAL ();
}

inline
void StreamInSM::physical_read (size_t& align, size_t& size, void* buf)
{
	uint8_t* dst = (uint8_t*)buf;
	do {
		const uint8_t* src;
		size_t cb;
		for (;;) {
			src = round_up (cur_ptr_, align);
			const uint8_t* end = (const uint8_t*)cur_block_ + cur_block_->size;
			const uint8_t* next_segment = nullptr;
			if (segments_ && (const uint8_t*)segments_ < end) {
				next_segment = (const uint8_t*)segments_;
				end = next_segment;
			}
			if (end < src)
				cb = 0;
			else
				cb = end - src;

			if (cb < align) {
				if (next_segment)
					return;
				next_block ();
			} else
				break;
		}

		if ((size_t)cb > size)
			cb = size;
		const uint8_t* end = src + cb;
		if (dst)
			dst = real_copy (src, end, dst);
		cur_ptr_ = end;
		size -= cb;
		inc_position (cb);
		// Adjust alignment if the remaining size less than it
		if (align > size)
			align = size;

	} while (size);
}

void StreamInSM::read (size_t align, size_t size, void* buf)
{
	if (!size)
		return;

	uint8_t* dst = (uint8_t*)buf;
	const Segment* segment = get_segment (align, size);
	do {
		if (segment) {
			size_t cb = segment->size;
			if (cb > size) // We can not stop reading in the middle of the segment.
				throw_MARSHAL (); // Data structure is not valid.
			if (dst) {
				Port::Memory::copy (dst, segment->pointer, cb, Memory::SRC_DECOMMIT);
				dst += cb;
			}
			Port::Memory::release (segment->pointer, segment->size);
			size -= cb;
			inc_position (cb);
		} else {
			physical_read (align, size, buf);
			if (size) {
				segment = get_segment (align, size);
				if (!segment)
					throw_MARSHAL (); // Something is wrong
			}
		}
	} while (size);
}

void* StreamInSM::read (size_t align, size_t& size)
{
	if (!size)
		return nullptr;

	void* ret = nullptr;
	const Segment* segment = get_segment (align, size);
	if (segment) {
		inc_position (size);
		// Adopt segment
		size = round_up (segment->size, Port::Memory::ALLOCATION_UNIT);
		ret = segment->pointer;
	} else {
		// Allocate buffer and read
		size_t cb_read = size;
		ret = MemContext::current ().heap ().allocate (nullptr, size, 0);
		try {
			read (align, cb_read, ret);
		} catch (...) {
			MemContext::current ().heap ().release (ret, size);
			throw;
		}
	}
	return ret;
}

void StreamInSM::set_size (size_t size)
{
	// set_size must not be used in ESIOP
	throw_BAD_OPERATION ();
}

size_t StreamInSM::end ()
{
	size_t rem_size = 0;
	if (cur_block_) {
		if (cur_block_->next)
			rem_size = std::numeric_limits <size_t>::max ();
		else {
			const uint8_t* block_end = (const uint8_t*)cur_block_ + cur_block_->size;
			rem_size = block_end - cur_ptr_;
		}
	}
	return rem_size;
}

size_t StreamInSM::position () const
{
	return position_;
}

size_t StreamInSM::chunk_tail () const
{
	assert (chunk_mode_);
	if (chunk_end_ <= position_)
		return 0;
	else
		return chunk_end_ - position_;
}

CORBA::Long StreamInSM::skip_chunks ()
{
	assert (chunk_mode_);
	while (chunk_end_ > position_) {
		read (1, chunk_end_ - position_, nullptr);
		chunk_mode_ = false;
		CORBA::Long l = read32 ();
		chunk_mode_ = true;
		if (l <= 0 || l >= 0x7FFFFF00)
			return l;
		chunk_end_ = position_ + l;
	}
	chunk_mode_ = false;
	CORBA::Long l = read32 ();
	chunk_mode_ = true;
	return l;
}

}
