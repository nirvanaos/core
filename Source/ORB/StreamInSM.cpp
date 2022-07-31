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

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Internal {
namespace Core {

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

void StreamInSM::read (size_t align, size_t size, void* buf)
{
	if (!size)
		return;
	if (!cur_block_)
		throw MARSHAL ();
	const Octet* src = round_up (cur_ptr_, align);
	Octet* dst = (Octet*)buf;
	do {
		const Octet* block_end = (const Octet*)cur_block_ + cur_block_->size;
		ptrdiff_t cb = block_end - src;
		if (cb < (ptrdiff_t)align) {
			BlockHdr* block = cur_block_;
			cur_block_ = cur_block_->next;
			Port::Memory::release (block, block->size);
			if (!cur_block_)
				throw MARSHAL ();
			cur_ptr_ = (const Octet*)(cur_block_ + 1);
		} else {
			if ((size_t)cb > size)
				cb = size;
			const Octet* end = src + cb;
			dst = real_copy (src, end, dst);
			src = end;
			size -= cb;
			// Adjust alignment if the remaining size less then it
			if (align < size)
				align = size;
		}
	} while (size);
}

void* StreamInSM::read (size_t align, size_t& size)
{
	if (!size)
		return nullptr;
	if (!cur_block_)
		throw MARSHAL ();
	if (segments_) {
		// Find potential segment
		const Segment* segment = (const Segment*)round_up (cur_ptr_, alignof (Segment));
		const Octet* block_end = (const Octet*)cur_block_ + cur_block_->size;
		if (block_end - (const Octet*)segment < sizeof (Segment)) {
			BlockHdr* next_block = cur_block_->next;
			if (next_block)
				segment = (const Segment*)round_up ((const Octet*)(next_block + 1), alignof (Segment));
			else
				segment = nullptr;
		}
		if (segment == segments_) {
			// Adopt segment
			segments_ = segment->next;
			cur_ptr_ = (const Octet*)(segment + 1);
			size = segment->allocated_size;
			return segment->pointer;
		}
	}

	// Allocate buffer and read
	size_t cb_read = size;
	void* buf = MemContext::current ().heap ().allocate (nullptr, size, 0);
	read (align, cb_read, buf);
	return buf;
}

}
}
}
