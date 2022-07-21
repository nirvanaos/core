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
					throw_MARSHAL ();
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
	invert_list (interfaces_);
	invert_list (segments_);
	reset ();
}

void RequestLocal::invert_list (void*& head)
{
	void* p = head;
	head = nullptr;
	while (p) {
		Element elem;
		set_ptr (p);
		read (alignof (void*), sizeof (Element), &elem);
		void* next = elem.next;
		elem.next = head;
		set_ptr (p);
		write (alignof (void*), sizeof (Element), &elem);
		head = (void*)p;
		p = next;
	}
}

void RequestLocal::cleanup () NIRVANA_NOEXCEPT
{
	{
		void* p = interfaces_;
		interfaces_ = nullptr;
		while (p) {
			set_ptr (p);
			ItfRecord itf;
			read (alignof (void*), sizeof (ItfRecord), &itf);
			interface_release ((Interface*)itf.ptr);
			p = itf.next;
		}
	}
	{
		void* p = segments_;
		segments_ = nullptr;
		while (p) {
			set_ptr (p);
			Segment seg;
			read (alignof (void*), sizeof (Segment), &seg);
			caller_memory_->heap ().release (seg.ptr, seg.allocated_size);
			p = seg.next;
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

void RequestLocal::check_align (size_t align)
{
	if (!(align == 1 || align == 2 || align == 4 || align == 8))
		throw_BAD_PARAM ();
}

void* RequestLocal::write (size_t align, size_t size, const void* data)
{
	if (!size)
		return nullptr;

	Octet* begin = nullptr;
	const Octet* src = (const Octet*)data;
	const Octet* block_end = cur_block_end ();
	Octet* dst = round_up (cur_ptr_, align);
	if (dst < block_end) {
		begin = dst;
		size_t cb = min ((size_t)(block_end - dst), size);
		real_copy (src, src + cb, dst);
		size -= cb;
		src += cb;
		dst += cb;
	}
	if (size) {
		// Allocate new block
		size_t block_size = max (BLOCK_SIZE, sizeof (BlockHdr) + size);
		BlockHdr* block = (BlockHdr*)caller_memory_->heap ().allocate (nullptr, block_size, 0);
		block->size = block_size;
		block->next = nullptr;
		if (!first_block_)
			first_block_ = block;
		if (cur_block_)
			cur_block_->next = block;
		cur_block_ = block;
		dst = (Octet*)(block + 1);
		if (!begin)
			begin = dst;
		real_copy (src, src + size, dst);
		dst += size;
	}
	cur_ptr_ = dst;
	return begin;
}

const void* RequestLocal::read (size_t align, size_t size, void* data)
{
	if (!size)
		return nullptr;

	const Octet* begin = nullptr;
	const Octet* src = round_up (cur_ptr_, align);
	const Octet* block_end = cur_block_end ();
	Octet* dst = (Octet*)data;

	for (;;) {
		if (src < block_end) {
			if (!begin)
				begin = src;
			size_t cb = min ((size_t)(block_end - src), size);
			real_copy (src, src + cb, dst);
			size -= cb;
			src += cb;
			dst += cb;
			if (!size)
				break;
		}

		// Go to the next block
		if (!cur_block_)
			cur_block_ = first_block_;
		else
			cur_block_ = cur_block_->next;
		if (!cur_block_)
			throw_MARSHAL ();
		block_end = (Octet*)cur_block_ + cur_block_->size;
		src = (Octet*)(cur_block_ + 1);
		if (!begin)
			begin = src;
	}

	cur_ptr_ = (Octet*)src;
	return begin;
}

void RequestLocal::set_ptr (const void* p)
{
	const Octet* block_begin = (const Octet*)this;
	const Octet* block_end = block_begin + BLOCK_SIZE;
	if (block_begin < p && p < block_end)
		cur_block_ = nullptr;
	else {
		BlockHdr* block = first_block_;
		while (block) {
			block_begin = (const Octet*)(block + 1);
			block_end = (const Octet*)block + block->size;
			if (block_begin < p && p < block_end)
				break;
			block = block->next;
		}
		assert (block);
		cur_block_ = block;
	}

	cur_ptr_ = (Octet*)p;
}

void RequestLocal::marshal_segment (size_t align, size_t element_size,
	size_t element_count, void* data, size_t allocated_size)
{
	assert (element_count);
	size_t size = element_count * round_up (element_size, align);
	if (allocated_size && allocated_size < size)
		throw_BAD_PARAM ();
	Segment segment;
	if (allocated_size && &caller_memory_->heap () == &callee_memory_->heap ()) {
		segment.allocated_size = allocated_size;
		segment.ptr = data;
	} else {
		Heap& tm = target_memory ();
		void* p = tm.copy (nullptr, data, size, 0);
		segment.allocated_size = size;
		segment.ptr = p;
		if (allocated_size)
			source_memory ().release (data, allocated_size);
	}
	segment.next = segments_;
	try {
		segments_ = write (alignof (void*), sizeof (Segment), &segment);
	} catch (...) {
		target_memory ().release (segment.ptr, segment.allocated_size);
		throw;
	}
}

void RequestLocal::unmarshal_segment (void*& data, size_t& allocated_size)
{
	Segment segment;
	const void* ps = read (alignof (void*), sizeof (Segment), &segment);
	if (segments_ != ps)
		throw_MARSHAL ();
	allocated_size = segment.allocated_size;
	data = segment.ptr;
	segments_ = segment.next;
}

void RequestLocal::marshal_interface (Interface::_ptr_type itf)
{
	marshal_op ();
	if (itf) {
		ItfRecord rec;
		rec.ptr = &itf;
		rec.next = interfaces_;
		interfaces_ = write (alignof (void*), sizeof (ItfRecord), &rec);
		interface_duplicate ((Interface*)rec.ptr); // If no exception in write
	} else {
		Interface* nil = nullptr;
		write (alignof (Interface*), sizeof (Interface*), &nil);
	}
}

Interface::_ref_type RequestLocal::unmarshal_interface (String_in interface_id)
{
	ItfRecord rec;
	const void* pr = read (alignof (void*), sizeof (Interface*), &rec.ptr);
	if (rec.ptr) {
		if (interfaces_ != pr)
			Nirvana::throw_MARSHAL ();
		read (alignof (void*), sizeof (void*), &rec.next);
		Interface::_check ((Interface*)rec.ptr, interface_id);
		interfaces_ = rec.next;
		return move ((Interface::_ref_type&)(rec.ptr));
	} else
		return nullptr;
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
