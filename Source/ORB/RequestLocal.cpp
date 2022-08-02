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
			if (!callee_memory_) {
				ExecDomain& ed = ExecDomain::current ();
				if (&ed.sync_context () == &proxy_->sync_context ())
					callee_memory_ = ed.mem_context_ptr ();
				if (!callee_memory_)
					throw MARSHAL ();
			}
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
	invert_list ((Element*&)interfaces_);
	invert_list ((Element*&)segments_);
	reset ();
}

void RequestLocal::invert_list (Element*& head)
{
	Element* p = head;
	Element* tail = nullptr;
	while (p) {
		Element* next = p->next;
		p->next = tail;
		tail = p;
		p = next;
	}
	head = tail;
}

void RequestLocal::cleanup () NIRVANA_NOEXCEPT
{
	{
		const ItfRecord* p = interfaces_;
		interfaces_ = nullptr;
		while (p) {
			interface_release ((Interface*)p->ptr);
			p = (const ItfRecord*)p->next;
		}
	}
	{
		const Segment* p = segments_;
		segments_ = nullptr;
		while (p) {
			caller_memory_->heap ().release (p->ptr, p->allocated_size);
			p = (const Segment*)p->next;
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

void RequestLocal::write (size_t align, size_t size, const void* data)
{
	if (!size)
		return;
	if (!data)
		throw BAD_PARAM ();

	const Octet* src = (const Octet*)data;
	const Octet* block_end = cur_block_end ();
	Octet* dst = round_up (cur_ptr_, align);
	ptrdiff_t cb = block_end - dst;
	if (cb >= (ptrdiff_t)align) {
		if ((size_t)cb > size)
			cb = size;
		const Octet* end = src + cb;
		dst = real_copy (src, end, dst);
		src = end;
		size -= cb;
		// Adjust alignment if the remaining size less than it
		if (align > size)
			align = size;
	}
	if (size) {
		allocate_block (align, size);
		dst = real_copy (src, src + size, cur_ptr_);
	}
	cur_ptr_ = dst;
}

void RequestLocal::allocate_block (size_t align, size_t size)
{
	size_t data_offset = round_up (sizeof (BlockHdr), align);
	size_t block_size = max (BLOCK_SIZE, data_offset + size);
	BlockHdr* block = (BlockHdr*)caller_memory_->heap ().allocate (nullptr, block_size, 0);
	block->size = block_size;
	block->next = nullptr;
	if (!first_block_)
		first_block_ = block;
	if (cur_block_)
		cur_block_->next = block;
	cur_block_ = block;
	cur_ptr_ = (Octet*)block + data_offset;
}

void RequestLocal::read (size_t align, size_t size, void* data)
{
	if (!size)
		return;

	Octet* dst = (Octet*)data;

	const Octet* src = round_up (cur_ptr_, align);
	for (;;) {
		const Octet* block_end = cur_block_end ();
		ptrdiff_t cb = block_end - src;
		if (cb >= (ptrdiff_t)align) {
			if ((size_t)cb > size)
				cb = size;
			const Octet* end = src + cb;
			dst = real_copy (src, end, dst);
			src = end;
			size -= cb;
			if (!size)
				break;
			// Adjust alignment if the remaining size less than it
			if (align > size)
				align = size;
		}

		next_block ();
		src = round_up (cur_ptr_, align);
	}

	cur_ptr_ = (Octet*)src;
}

void RequestLocal::next_block ()
{
	if (!cur_block_)
		cur_block_ = first_block_;
	else
		cur_block_ = cur_block_->next;
	if (!cur_block_)
		throw MARSHAL ();
	cur_ptr_ = (Octet*)(cur_block_ + 1);
}

RequestLocal::Element* RequestLocal::get_element_buffer (size_t size)
{
	const Octet* block_end = cur_block_end ();
	Octet* dst = round_up (cur_ptr_, alignof (Element));
	ptrdiff_t cb = block_end - dst;
	if (cb < (ptrdiff_t)size) {
		allocate_block (alignof (Element), size);
		dst = cur_ptr_;
	}
	cur_ptr_ = dst + size;
	return (Element*)dst;
}

void RequestLocal::marshal_segment (size_t align, size_t element_size,
	size_t element_count, void* data, size_t allocated_size)
{
	assert (element_count);
	size_t size = element_count * round_up (element_size, align);
	if (allocated_size && allocated_size < size)
		throw BAD_PARAM ();
	Segment* segment = (Segment*)get_element_buffer (sizeof (Segment));
	if (allocated_size && &caller_memory_->heap () == &callee_memory_->heap ()) {
		segment->allocated_size = allocated_size;
		segment->ptr = data;
	} else {
		Heap& tm = target_memory ();
		void* p = tm.copy (nullptr, data, size, 0);
		segment->allocated_size = size;
		segment->ptr = p;
		if (allocated_size)
			source_memory ().release (data, allocated_size);
	}
	segment->next = segments_;
	segments_ = segment;
}

void RequestLocal::unmarshal_segment (void*& data, size_t& allocated_size)
{
	if (!segments_)
		throw MARSHAL ();

	const Segment* segment = (const Segment*)round_up (cur_ptr_, alignof (Element));
	const Octet* block_end = cur_block_end ();
	if (block_end - (const Octet*)segment < sizeof (Segment)) {
		next_block ();
		segment = (const Segment*)round_up (cur_ptr_, alignof (Element));
	}
	if (segments_ != segment)
		throw MARSHAL ();
	segments_ = (Segment*)(segment->next);
	cur_ptr_ = (Octet*)(segment + 1);
	allocated_size = segment->allocated_size;
	data = segment->ptr;
}

void RequestLocal::marshal_interface (Interface::_ptr_type itf)
{
	marshal_op ();
	if (itf) {
		const Octet* block_end = cur_block_end ();
		Octet* dst = round_up (cur_ptr_, alignof (Interface*));
		ptrdiff_t tail = block_end - dst;
		if (sizeof (Interface*) <= tail && tail < sizeof (ItfRecord)) {
			// Mark interface non-null
			*(Interface**)dst = &itf;
		}
		ItfRecord* itf_rec = (ItfRecord*)get_element_buffer (sizeof (ItfRecord));
		itf_rec->ptr = interface_duplicate (&itf);
		itf_rec->next = interfaces_;
		interfaces_ = itf_rec;
	} else {
		Interface* nil = nullptr;
		write (alignof (Interface*), sizeof (Interface*), &nil);
	}
}

Interface::_ref_type RequestLocal::unmarshal_interface (String_in interface_id)
{
	Interface** pitf = (Interface**)round_up (cur_ptr_, alignof (Interface*));
	const Octet* block_end = cur_block_end ();
	if (block_end - (const Octet*)pitf < sizeof (Interface*)) {
		next_block ();
		pitf = (Interface**)round_up (cur_ptr_, alignof (Interface*));
		block_end = cur_block_end ();
	}

	if (!*pitf) {
		cur_ptr_ = (Octet*)(pitf + 1);
		return nullptr;
	}
		
	if (!interfaces_)
		throw MARSHAL ();
	if (block_end - (const Octet*)pitf < sizeof (ItfRecord))
		next_block ();
	ItfRecord* itf_rec = (ItfRecord*)round_up (cur_ptr_, alignof (Element));
	if (interfaces_ != itf_rec)
		throw MARSHAL ();

	Interface::_check ((Interface*)itf_rec->ptr, interface_id);
	interfaces_ = (ItfRecord*)(itf_rec->next);
	cur_ptr_ = (Octet*)(itf_rec + 1);
	return move ((Interface::_ref_type&)(itf_rec->ptr));
}

void RequestLocal::marshal_value_copy (ValueBase::_ptr_type base, String_in interface_id)
{
	ValueBase::_ref_type copy = base->_copy_value ();
	Interface::_ptr_type itf = copy->_query_valuetype (interface_id);
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
