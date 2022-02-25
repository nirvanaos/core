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
#include "RequestLocal.h"

namespace CORBA {
namespace Internal {
namespace Core {

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace std;

RqKind RequestLocal::kind () const NIRVANA_NOEXCEPT
{
	return RqKind::SYNC;
}

Heap& RequestLocal::target_memory ()
{
	switch (state_) {
		case State::CALLER:
		case State::CALL:
			if (!callee_memory_)
				callee_memory_ = MemContextUser::create ();
			return callee_memory_->heap ();

		default:
			return caller_memory_->heap ();
	}
}

Heap& RequestLocal::source_memory ()
{
	switch (state_) {
		case State::CALLER:
		case State::CALL:
			return caller_memory_->heap ();

		default:
			if (!callee_memory_)
				throw_MARSHAL ();
			return callee_memory_->heap ();
	}
}

void RequestLocal::marshal_op () NIRVANA_NOEXCEPT
{
	if (State::CALL == state_) {
		clear ();
		state_ = State::CALLEE;
	}
}

void RequestLocal::rewind () NIRVANA_NOEXCEPT
{
	invert_list (interfaces_);
	invert_list (segments_);
	reset ();
}

void RequestLocal::cleanup () NIRVANA_NOEXCEPT
{
	{
		ItfRecord* itf = interfaces_;
		interfaces_ = nullptr;
		while (itf) {
			interface_release (itf->ptr);
			itf = itf->next;
		}
	}
	{
		Segment* s = segments_;
		segments_ = nullptr;
		while (s) {
			caller_memory_->heap ().release (s->allocated_memory, s->allocated_size);
			s = s->next;
		}
	}
	{
		BlockHdr* block = first_block_;
		if (block) {
			first_block_ = nullptr;
			Heap& mem = caller_memory_->heap ();
			while (block) {
				BlockHdr* next = block->next;
				mem.release (block, block->size);
				block = next;
			}
		}
	}
}

void* RequestLocal::allocate_space (size_t align, size_t size, size_t reserve)
{
	if (!align || align > alignof (max_align_t) || numeric_limits <size_t>::max () - align < reserve)
		throw_BAD_PARAM ();
	if (!reserve)
		return nullptr;

	Octet* aligned = round_up (cur_ptr_, align);
	if (cur_block_end () < aligned + reserve) {
		// Allocate new block
		size_t block_align = round_up (sizeof (BlockHdr), align);
		size_t block_size = max (BLOCK_SIZE, block_align + reserve);
		BlockHdr* block = (BlockHdr*)caller_memory_->heap ().allocate (nullptr, block_size, 0);
		block->size = block_size;
		block->next = nullptr;
		if (!first_block_)
			first_block_ = block;
		if (cur_block_)
			cur_block_->next = block;
		cur_block_ = block;
		aligned = (Octet*)block + block_align;
	}
	cur_ptr_ = aligned + size;
	return aligned;
}

void* RequestLocal::get_data (size_t align, size_t size, size_t reserve)
{
	if (!align || align > alignof (max_align_t) || numeric_limits <size_t>::max () - align < reserve)
		throw_BAD_PARAM ();

	Octet* aligned = round_up (cur_ptr_, align);
	const Octet* block_end = cur_block_end ();
	if (cur_ptr_ != block_end) {
		if (block_end < aligned + reserve) {
			// Empty space may be only if block has default size
			if (cur_block_ && cur_block_->size != BLOCK_SIZE)
				throw_MARSHAL ();
		} else {
			cur_ptr_ = aligned + size;
			return aligned;
		}
	}

	// Go to the next block
	if (!cur_block_)
		cur_block_ = first_block_;
	else
		cur_block_ = cur_block_->next;
	if (!cur_block_)
		throw_MARSHAL ();
	block_end = (Octet*)cur_block_ + cur_block_->size;
	cur_ptr_ = (Octet*)(cur_block_ + 1);
	aligned = round_up (cur_ptr_, align);
	if (block_end < aligned + reserve)
		throw_MARSHAL ();
	cur_ptr_ = aligned + size;
	return aligned;
}

void RequestLocal::marshal_seq (size_t align, size_t element_size,
	size_t element_count, void* data, size_t allocated_size,
	size_t max_inline_string)
{
	marshal_op ();

	*(size_t*)allocate_space (alignof (size_t), sizeof (size_t), sizeof (size_t)) = element_count;

	if (element_count) {
		size_t size = element_count * element_size;
		if (allocated_size && allocated_size < size)
			throw_MARSHAL ();
		if (element_count <= max_inline_string) {
			*(size_t*)allocate_space (alignof (Segment), sizeof (size_t), sizeof (Segment)) = 0;
			real_copy ((const Octet*)data, (const Octet*)data + size, (Octet*)allocate_space (align, size, size));
			if (allocated_size)
				source_memory ().release (data, allocated_size);
		} else {
			Segment* segment = (Segment*)allocate_space (alignof (Segment), sizeof (Segment), sizeof (Segment));
			if (allocated_size && &caller_memory_->heap () == &callee_memory_->heap ()) {
				segment->allocated_size = allocated_size;
				segment->allocated_memory = data;
			} else {
				Heap& tm = target_memory ();
				if (max_inline_string)
					size += element_size;
				void* p = tm.copy (nullptr, data, size, 0);
				uintptr_t au = tm.query (p, Memory::QueryParam::ALLOCATION_UNIT);
				segment->allocated_size = round_up (size, au);
				segment->allocated_memory = p;
				if (allocated_size)
					source_memory ().release (data, allocated_size);
			}
			segment->next = segments_;
			segments_ = segment;
		}
	}
}

bool RequestLocal::unmarshal_seq (size_t align, size_t element_size,
	size_t& element_count, void*& data, size_t& allocated_size)
{
	size_t cnt = *(size_t*)get_data (alignof (size_t), sizeof (size_t), sizeof (size_t));
	if (cnt) {
		size_t size = element_size * cnt;
		Segment* segment = (Segment*)get_data (alignof (Segment), sizeof (segment->allocated_size), sizeof (Segment));
		size_t allocated = segment->allocated_size;
		if (allocated) {
			if (segments_ != segment || allocated < size)
				throw_MARSHAL ();
			allocated_size = allocated;
			size_t rem = sizeof (Segment) - sizeof (segment->allocated_size);
			get_data (1, rem, rem);
			data = segment->allocated_memory;
			segments_ = segment->next;
		} else {
			allocated_size = 0;
			data = get_data (align, size, size);
		}
	} else {
		allocated_size = 0;
		data = nullptr;
	}
	element_count = cnt;
	return false;
}

void RequestLocal::marshal_interface (Interface::_ptr_type itf)
{
	marshal_op ();
	if (itf) {
		ItfRecord* rec = (ItfRecord*)allocate_space (alignof (ItfRecord), sizeof (ItfRecord), sizeof (ItfRecord));
		rec->ptr = interface_duplicate (&itf);
		rec->next = interfaces_;
		interfaces_ = rec;
	} else
		*(Interface**)allocate_space (alignof (ItfRecord*), sizeof (Interface*), sizeof (ItfRecord)) = nullptr;
}

Interface::_ref_type RequestLocal::unmarshal_interface (String_in interface_id)
{
	ItfRecord* rec = (ItfRecord*)get_data (alignof (ItfRecord), sizeof (rec->ptr), sizeof (ItfRecord));
	if (rec->ptr) {
		if (interfaces_ != rec)
			Nirvana::throw_MARSHAL ();
		size_t rem = sizeof (ItfRecord) - sizeof (rec->ptr);
		get_data (1, rem, rem);
		Interface::_check (rec->ptr, interface_id);
		interfaces_ = rec->next;
		return move ((Interface::_ref_type&)(rec->ptr));
	} else
		return nullptr;
}

void RequestLocal::marshal_value_copy (ValueBase::_ptr_type base, String_in interface_id)
{
	ValueBase::_ref_type copy = base->_copy_value ();
	Interface::_ptr_type itf = AbstractBase::_ptr_type (copy)->_query_interface (interface_id);
	if (!itf)
		Nirvana::throw_MARSHAL ();
	marshal_interface (itf);
}

RqKind RequestLocalAsync::kind () const NIRVANA_NOEXCEPT
{
	return RqKind::ASYNC;
}

void RequestLocalAsync::run ()
{
	invoke ();
}

RqKind RequestLocalOneway::kind () const NIRVANA_NOEXCEPT
{
	return RqKind::ONEWAY;
}

}
}
}
