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
#include "../Chrono.h"
#include "../MemContextImpl.h"
#include "../virtual_copy.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestLocalBase::RequestLocalBase (MemContext* callee_memory, unsigned response_flags)
noexcept :
	caller_memory_ (&MemContext::current ()),
	callee_memory_ (callee_memory),
	state_ (State::CALLER),
	response_flags_ ((uint8_t)response_flags),
	cancelled_ (false),
	first_block_ (nullptr),
	cur_block_ (nullptr),
	interfaces_ (nullptr),
	segments_ (nullptr)
{
	assert ((uintptr_t)this % BLOCK_SIZE == 0);
}

MemContext& RequestLocalBase::target_memory ()
{
	switch (state_) {
		case State::CALLER:
		case State::CALL:
			if (!callee_memory_)
				callee_memory_ = MemContextImpl::create ();
			return *callee_memory_;

		default:
			return *caller_memory_;
	}
}

MemContext& RequestLocalBase::source_memory ()
{
	switch (state_) {
		case State::CALLER:
		case State::CALL:
			return *caller_memory_;

		default:
			if (!callee_memory_) {
				callee_memory_ = ExecDomain::current ().mem_context_ptr ();
				if (!callee_memory_)
					throw MARSHAL ();
			}
			return *callee_memory_;
	}
}

bool RequestLocalBase::marshal_op () noexcept
{
	if (State::CALL == state_) {
		// Leave sync domain, if any.
		// Output data marshaling performed out of sync domain.
		ExecDomain::current ().leave_sync_domain ();

		clear ();
		state_ = State::CALLEE;
	}
	return state_ == State::CALLER || (response_flags_ & RESPONSE_DATA)
		|| (state_ == State::EXCEPTION && response_flags_);
}

void RequestLocalBase::rewind () noexcept
{
	invert_list ((Element*&)interfaces_);
	invert_list ((Element*&)segments_);
	reset ();
}

void RequestLocalBase::invert_list (Element*& head)
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

void RequestLocalBase::cleanup () noexcept
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

void RequestLocalBase::write (size_t align, size_t size, const void* data)
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
		virtual_copy (src, cb, dst);
		dst += cb;
		src += cb;
		size -= cb;
		// Adjust alignment if the remaining size less than it
		if (align > size)
			align = size;
	}
	if (size) {
		allocate_block (align, size);
		dst = sizeof (BlockHdr) >= 8 ? cur_ptr_ : round_up (cur_ptr_, align);
		virtual_copy (src, size, dst);
		dst += size;
	}
	cur_ptr_ = dst;
}

void RequestLocalBase::allocate_block (size_t align, size_t size)
{
	size_t data_offset = round_up (sizeof (BlockHdr), align);
	size_t block_size = std::max (BLOCK_SIZE, data_offset + size);
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

void RequestLocalBase::read (size_t align, size_t size, void* data)
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
			virtual_copy (src, cb, dst);
			dst += cb;
			src += cb;
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

void RequestLocalBase::next_block ()
{
	if (!cur_block_)
		cur_block_ = first_block_;
	else
		cur_block_ = cur_block_->next;
	if (!cur_block_)
		throw MARSHAL (MARSHAL_MINOR_FEWER);
	cur_ptr_ = (Octet*)(cur_block_ + 1);
}

RequestLocal::Element* RequestLocalBase::get_element_buffer (size_t size)
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

void RequestLocalBase::marshal_segment (size_t align, size_t element_size,
	size_t element_count, void* data, size_t& allocated_size)
{
	assert (element_count);
	size_t size = element_count * round_up (element_size, align);
	if (allocated_size && allocated_size < size)
		throw BAD_PARAM ();
	Segment* segment = (Segment*)get_element_buffer (sizeof (Segment));
	if (allocated_size && &caller_memory_->heap () == &callee_memory_->heap ()) {
		segment->allocated_size = allocated_size;
		segment->ptr = data;
		allocated_size = 0;
	} else {
		Heap& target_heap = target_memory ().heap ();
		if (allocated_size) {
			Heap& source_heap = source_memory ().heap ();
			segment->ptr = target_heap.move_from (source_heap, data, size);
			size_t cb_release = allocated_size - size;
			allocated_size = 0;
			if (cb_release)
				source_heap.release ((Octet*)data + size, cb_release);
		} else
			segment->ptr = target_heap.copy (nullptr, data, size, 0);
		segment->allocated_size = size;
	}
	segment->next = segments_;
	segments_ = segment;
}

void RequestLocalBase::unmarshal_segment (void*& data, size_t& allocated_size)
{
	if (!segments_)
		throw MARSHAL (MARSHAL_MINOR_FEWER);

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

	void* p = segment->ptr;
	size_t size = segment->allocated_size;

	Heap& cur_heap = MemContext::current ().heap ();
	Heap& target_heap = target_memory ().heap ();
	if (&cur_heap != &target_heap) {
		try {
			p = cur_heap.move_from (target_heap, p, size);
		} catch (...) {
			target_heap.release (p, size);
			throw;
		}
	}
	allocated_size = size;
	data = p;
}

void RequestLocalBase::marshal_interface (Interface::_ptr_type itf)
{
	if (marshal_op ()) {
		if (itf)
			marshal_interface_internal (itf);
		else {
			Interface* nil = nullptr;
			write (alignof (Interface*), sizeof (Interface*), &nil);
		}
	}
}

void RequestLocalBase::marshal_interface_internal (Interface::_ptr_type itf)
{
	assert (itf);
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
}

Interface::_ref_type RequestLocalBase::unmarshal_interface (const IDL::String& interface_id)
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
		throw MARSHAL (MARSHAL_MINOR_FEWER);
	if (block_end - (const Octet*)pitf < sizeof (ItfRecord))
		next_block ();
	ItfRecord* itf_rec = (ItfRecord*)round_up (cur_ptr_, alignof (Element));
	if (interfaces_ != itf_rec)
		throw MARSHAL ();

	Interface::_check ((Interface*)itf_rec->ptr, interface_id);
	interfaces_ = (ItfRecord*)(itf_rec->next);
	cur_ptr_ = (Octet*)(itf_rec + 1);
	return std::move ((Interface::_ref_type&)(itf_rec->ptr));
}

void RequestLocalBase::marshal_value (ValueBase::_ptr_type base, const IDL::String& interface_id, bool output)
{
	assert (base);
	ValueFactoryBase::_ref_type factory = base->_factory ();
	if (!factory)
		throw MARSHAL (MAKE_OMG_MINOR (1)); // Try to marshal abstract value

	auto ins = value_map_.marshal_map ().emplace (&base, (uintptr_t)cur_ptr_ + 1);
	if (!ins.second) {
		// This value was already marshalled, write indirection tag.
		write8 (3);
		write (alignof (uintptr_t), sizeof (uintptr_t), &ins.first->second);
	} else {
		write8 (2);
		marshal_interface_internal (ValueFactoryBase::_ptr_type (factory));
		if (output)
			base->_marshal_out (_get_ptr ());
		else
			base->_marshal_in (_get_ptr ());
	}
}

Interface::_ref_type RequestLocalBase::unmarshal_value (const IDL::String& interface_id)
{
	Internal::Interface::_ref_type ret;
	uint8_t tag = read8 ();
	switch (tag) {
	case 1:
		ret = unmarshal_interface (interface_id);
		break;

	case 2: {
		uintptr_t pos = (uintptr_t)cur_ptr_;
		ValueFactoryBase::_ref_type factory = unmarshal_interface (Internal::RepIdOf <ValueFactoryBase>::id).template downcast <ValueFactoryBase> ();
		if (!factory)
			throw MARSHAL (); // Unexpected
		ValueBase::_ref_type base (factory->create_for_unmarshal ());
		base->_unmarshal (_get_ptr ());
		value_map_.unmarshal_map ().emplace (pos, &ValueBase::_ptr_type (base));
		ret = base->_query_valuetype (interface_id);
	} break;

	case 3: {
		uintptr_t p;
		read (alignof (uintptr_t), sizeof (uintptr_t), &p);
		const auto& unmarshal_map = value_map_.unmarshal_map ();
		Interface* vb = unmarshal_map.find (p);
		if (!vb)
			throw MARSHAL ();
		ret = static_cast <ValueBase*> (vb)->_query_valuetype (interface_id);
	} break;

	}

	if (tag && !ret)
		throw MARSHAL (); // Unexpected

	return ret;
}

void RequestLocalBase::invoke ()
{
	rewind ();
	state_ = State::CALL;
}

RequestLocal::RequestLocal (ProxyManager& proxy, Internal::IOReference::OperationIndex op_idx,
	Nirvana::Core::MemContext* callee_memory, unsigned response_flags) noexcept :
	RequestLocalBase (callee_memory, response_flags),
	proxy_ (&proxy),
	op_idx_ (op_idx)
{}

void RequestLocal::invoke ()
{
	RequestLocalBase::invoke ();
	// We don't need to handle exceptions here, because invoke_sync ()
	// does not throw exceptions.
	Nirvana::Core::Synchronized _sync_frame (proxy ()->get_sync_context (op_idx ()), memory ());
	invoke_sync ();
}

void RequestLocal::invoke_sync () noexcept
{
	assert (State::CALL == state ()); // RequestLocalBase::invoke () must be called
	proxy ()->invoke (op_idx (), _get_ptr ());
}

void RequestLocalOneway::invoke ()
{
	RequestLocalBase::invoke (); // rewind
	ExecDomain::async_call <Runnable> (ExecDomain::current ().get_request_deadline (!response_flags ()),
		proxy ()->get_sync_context (op_idx ()), memory (), std::ref (*this));
}

void RequestLocalOneway::Runnable::run ()
{
	request_->run ();
}

void RequestLocalAsync::cancel () noexcept
{
	if (set_cancelled ())
		response_flags_ = 0; // Prevent marshaling
}

void RequestLocalAsync::finalize () noexcept
{
	Base::finalize ();
	RqHelper::call_completed (callback_, _get_ptr ());
}

}
}
