/// \file
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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCAL_H_
#define NIRVANA_ORB_CORE_REQUESTLOCAL_H_
#pragma once

#include "ServantProxyBase.h"
#include "../ExecDomain.h"
#include "../MemContext.h"
#include "IDL/IORequest_s.h"
#include "RqHelper.h"

namespace CORBA {
namespace Internal {
namespace Core {

class ServantProxyBase;

class RequestLocal :
	public servant_traits <IORequest>::Servant <RequestLocal>,
	public LifeCycleRefCnt <RequestLocal>,
	public Nirvana::Core::UserObject,
	private RqHelper
{
public:
	static const size_t BLOCK_SIZE = (32 * sizeof (size_t)
		+ alignof (max_align_t) - 1) / alignof (max_align_t) * alignof (max_align_t);

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	virtual void _remove_ref () NIRVANA_NOEXCEPT = 0;

	IOReference::OperationIndex op_idx () const NIRVANA_NOEXCEPT
	{
		return op_idx_;
	}

	Nirvana::Core::MemContext* memory () const NIRVANA_NOEXCEPT
	{
		return callee_memory_;
	}

	virtual RqKind kind () const NIRVANA_NOEXCEPT;

	///@{
	/// Marshal/unmarshal data that meet the common data representation.

	/// Marshal CDR data.
	/// 
	/// \param align Data alignment
	/// \param size  Data size.
	/// \param data  Pointer to the data with common-data-representation (CDR).
	void marshal (size_t align, size_t size, const void* data)
	{
		marshal_op ();
		Nirvana::real_copy ((const Octet*)data, (const Octet*)data + size, (Octet*)allocate_space (align, size, size));
	}

	/// Unmarshal CDR data.
	/// 
	/// \param align Data alignment
	/// \param size  Data size.
	/// \param [out] data Pointer to the internal data buffer.
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	bool unmarshal (size_t align, size_t size, void*& data)
	{
		data = get_data (align, size, size);
		return false;
	}

	/// Marshal CDR sequence.
	/// 
	/// \param align Data alignment
	/// \param element_size Element size.
	/// \param element_count Count of elements.
	/// \param data Pointer to the data with common-data-representation (CDR).
	/// \param allocated_size If this parameter is not zero, the request
	///        object becomes an owner of the memory block.
	void marshal_seq (size_t align, size_t element_size,
		size_t element_count, void* data, size_t allocated_size,
		size_t max_inline_string = 0);

	/// Unmarshal CDR sequence.
	/// 
	/// \param align Data alignment
	/// \param element_size Element size.
	/// \param [out] element_count Count of elements.
	/// \param [out] data Pointer to the data with common-data-representation (CDR).
	/// \param [out] allocated_size If this parameter is not zero, the caller
	///              becomes an owner of the memory block.
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	bool unmarshal_seq (size_t align, size_t element_size,
		size_t& element_count, void*& data, size_t& allocated_size);

	///@}

	///@{
	/// Marshal/unmarshal sequences.

	/// Write marshaling sequence size.
	/// 
	/// \param element_count The sequence size.
	void marshal_seq_begin (size_t element_count)
	{
		marshal_op ();
		*(size_t*)allocate_space (alignof (size_t), sizeof (size_t), sizeof (size_t)) = element_count;
	}

	/// Get unmarshalling sequence size.
	/// 
	/// \returns The sequence size.
	size_t unmarshal_seq_begin ()
	{
		return *(size_t*)get_data (alignof (size_t), sizeof (size_t), sizeof (size_t));
	}

	///@}

	///@{
	/// Marshal/unmarshal character data.

	template <typename C>
	void marshal_char (size_t count, const C* data)
	{
		marshal (alignof (C), count * sizeof (C), data);
	}

	template <typename C>
	void unmarshal_char (size_t count, C* data)
	{
		size_t cb = count * sizeof (C);
		const C* chars = (const C*)get_data (alignof (C), cb, cb);
		Nirvana::real_copy (chars, chars + count, data);
	}

	template <typename C>
	void marshal_string (StringT <C>& s, bool move)
	{
		typedef typename Type <StringT <C> >::ABI ABI;
		ABI& abi = (ABI&)s;
		size_t size;
		C* ptr;
		if (abi.is_large ()) {
			size = abi.large_size ();
			ptr = abi.large_pointer ();
		} else {
			size = abi.small_size ();
			ptr = abi.small_pointer ();
		}
		marshal_seq (alignof (C), sizeof (C), size, ptr,
			move ? abi.allocated () : 0, ABI::SMALL_CAPACITY);
		if (move)
			abi.reset ();
	}

	template <typename C>
	void unmarshal_string (StringT <C>& s)
	{
		size_t size, allocated;
		void* ptr;
		unmarshal_seq (alignof (C), sizeof (C), size, ptr, allocated);
		if (allocated) {
			s.clear ();
			s.shrink_to_fit ();
			typedef typename Type <StringT <C> >::ABI ABI;
			ABI& abi = (ABI&)s;
			abi.large_size (size);
			abi.allocated (allocated);
			abi.large_pointer ((C*)ptr);
		} else
			s.assign ((const C*)ptr, size);
	}

	template <typename C>
	void marshal_char_seq (Sequence <C>& s, bool move)
	{
		typedef typename Type <Sequence <C> >::ABI ABI;
		ABI& abi = (ABI&)s;
		marshal_seq (alignof (C), sizeof (C), abi.size, abi.ptr,
			move ? abi.allocated : 0);
		if (move)
			abi.reset ();
	}

	template <typename C>
	void unmarshal_char_seq (Sequence <C>& s)
	{
		size_t size, allocated;
		void* ptr;
		unmarshal_seq (alignof (C), sizeof (C), size, ptr, allocated);
		if (allocated) {
			s.clear ();
			s.shrink_to_fit ();
			typedef typename Type <Sequence <C> >::ABI ABI;
			ABI& abi = (ABI&)s;
			abi.size = size;
			abi.allocated = allocated;
			abi.ptr = (C*)ptr;
		} else
			s.assign ((const C*)ptr, (const C*)ptr + size);
	}

	void marshal_wchar (size_t count, const WChar* data)
	{
		marshal_char (count, data);
	}

	void unmarshal_wchar (size_t count, WChar* data)
	{
		unmarshal_char (count, data);
	}

	void marshal_wstring (WString& s, bool move)
	{
		marshal_string (s, move);
	}

	void unmarshal_wstring (WString& s)
	{
		unmarshal_string (s);
	}

	void marshal_wchar_seq (WCharSeq& s, bool move)
	{
		marshal_char_seq (s, move);
	}

	void unmarshal_wchar_seq (WCharSeq& s)
	{
		unmarshal_char_seq (s);
	}

	///@}

	///@{
	/// Object operations.

	/// Marshal interface.
	/// 
	/// \param itf The interface derived from Object or LocalObject.
	void marshal_interface (Interface::_ptr_type itf);

	/// Unmarshal interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	Interface::_ref_type unmarshal_interface (String_in interface_id);

	/// Marshal TypeCode.
	/// 
	/// \param tc TypeCode.
	void marshal_type_code (TypeCode::_ptr_type tc)
	{
		marshal_interface (tc);
	}

	/// Unmarshal TypeCode.
	/// 
	/// \returns TypeCode.
	TypeCode::_ref_type unmarshal_type_code ()
	{
		return unmarshal_interface (RepIdOf <TypeCode>::id).template downcast <TypeCode> ();
	}

	/// Marshal value type.
	/// 
	/// \param val  ValueBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_value (Interface::_ptr_type val, bool output)
	{
		if (output || !val || caller_memory_ == callee_memory_)
			marshal_interface (val);
		else
			marshal_value_copy (value_type2base (val), val->_epv ().interface_id);
	}

	void marshal_value_copy (ValueBase::_ptr_type base, String_in interface_id);

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// 
	/// \returns Value type interface.
	Interface::_ref_type unmarshal_value (String_in interface_id)
	{
		return unmarshal_interface (interface_id);
	}

	/// Marshal abstract interface.
	/// 
	/// \param itf The interface derived from AbstractBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_abstract (Interface::_ptr_type itf, bool output)
	{
		if (output || !itf || caller_memory_ == callee_memory_)
			marshal_interface (itf);
		else {
			// Downcast to AbstractBase
			AbstractBase::_ptr_type base = abstract_interface2base (itf);
			if (base->_to_object ())
				marshal_interface (itf);
			else {
				ValueBase::_ref_type value = base->_to_value ();
				if (!value)
					Nirvana::throw_MARSHAL ();
				marshal_value_copy (value, itf->_epv ().interface_id);
			}
		}
	}

	/// Unmarshal abstract interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	Interface::_ref_type unmarshal_abstract (String_in interface_id)
	{
		return unmarshal_interface (interface_id);
	}

	///@}

	///@{
	/// Callee operations.

	/// End of input data marshaling.
	/// 
	/// Marshaling resources may be released.
	/// This operation is called only if the input data is not empty.
	void unmarshal_end () NIRVANA_NOEXCEPT
	{
		clear ();
		state_ = State::CALLEE;
	}

	/// Return exception to caller.
	/// Operation has move semantics so `e` may be cleared.
	void set_exception (Any& e)
	{
		clear ();
		Type <Any>::marshal_out (e, _get_ptr ());
		rewind ();
		state_ = State::EXCEPTION;
	}

	/// Marks request as successful.
	void success ()
	{
		rewind ();
		state_ = State::SUCCESS;
	}

	///@}

	///@{
	/// Caller operations.

	/// Invoke request.
	/// 
	void invoke ()
	{
		rewind ();
		state_ = State::CALL;
		proxy_->invoke (*this);
	}

	bool completed () const
	{
		return State::EXCEPTION == state_ || State::SUCCESS == state_;
	}

	bool is_exception () const
	{
		return State::EXCEPTION == state_;
	}

	void wait ()
	{}

	///@}

protected:
	using MemContext = Nirvana::Core::MemContext;

	template <class T>
	using CoreRef = Nirvana::Core::CoreRef <T>;

	enum class State
	{
		CALLER,
		CALL,
		CALLEE,
		SUCCESS,
		EXCEPTION
	};

	RequestLocal (ServantProxyBase& proxy, IOReference::OperationIndex op_idx,
		Nirvana::Core::MemContext* callee_memory) NIRVANA_NOEXCEPT :
		caller_memory_ (&MemContext::current ()),
		callee_memory_ (callee_memory),
		proxy_ (&proxy),
		op_idx_ (op_idx),
		state_ (State::CALLER),
		first_block_ (nullptr),
		cur_block_ (nullptr),
		interfaces_ (nullptr),
		segments_ (nullptr)
	{
		assert ((uintptr_t)this % BLOCK_SIZE == 0);
	}

	Nirvana::Core::Heap& target_memory ();
	Nirvana::Core::Heap& source_memory ();

	const Octet* cur_block_end () const NIRVANA_NOEXCEPT
	{
		if (!cur_block_)
			return (const Octet*)this + BLOCK_SIZE;
		else
			return (const Octet*)cur_block_ + cur_block_->size;
	}

	void marshal_op () NIRVANA_NOEXCEPT;
	void* allocate_space (size_t align, size_t size, size_t reserve);
	void* get_data (size_t align, size_t size, size_t reserve);

	void rewind () NIRVANA_NOEXCEPT;

	virtual void reset () NIRVANA_NOEXCEPT
	{
		cur_block_ = nullptr;
	}

	void clear () NIRVANA_NOEXCEPT
	{
		cleanup ();
		reset ();
	}

	void cleanup () NIRVANA_NOEXCEPT;

	template <typename Elem>
	void invert_list (Elem*& head)
	{
		Elem* p = head;
		head = nullptr;
		while (p) {
			Elem* next = p->next;
			p->next = head;
			head = p;
			p = next;
		}
	}

protected:
	Nirvana::Core::RefCounter ref_cnt_;
	CoreRef <MemContext> caller_memory_;
	CoreRef <MemContext> callee_memory_;
	Octet* cur_ptr_;

private:
	struct BlockHdr
	{
		size_t size;
		BlockHdr* next;
	};

	struct Segment
	{
		size_t allocated_size;

		// If allocated_size is not zero, following members are present.
		// Otherwise, the sequence data are follows immediate
		// after allocated_size.
		void* allocated_memory;
		Segment* next;
	};

	struct ItfRecord
	{
		Interface* ptr;
		// next is present if ptr != nullptr
		ItfRecord* next;
	};

	CoreRef <ServantProxyBase> proxy_;
	IOReference::OperationIndex op_idx_;
	State state_;
	BlockHdr* first_block_;
	BlockHdr* cur_block_;
	ItfRecord* interfaces_;
	Segment* segments_;
};

class NIRVANA_NOVTABLE RequestLocalAsync :
	public RequestLocal,
	public Nirvana::Core::Runnable
{
protected:
	RequestLocalAsync (ServantProxyBase& proxy, IOReference::OperationIndex op_idx,
		Nirvana::Core::MemContext* callee_memory) NIRVANA_NOEXCEPT :
		RequestLocal (proxy, op_idx, callee_memory)
	{}

	virtual RqKind kind () const NIRVANA_NOEXCEPT;

	void _add_ref () NIRVANA_NOEXCEPT
	{
		RequestLocal::_add_ref ();
	}

private:
	virtual void run ();

};

class NIRVANA_NOVTABLE RequestLocalOneway :
	public RequestLocalAsync
{
protected:
	RequestLocalOneway (ServantProxyBase& proxy, IOReference::OperationIndex op_idx,
		Nirvana::Core::MemContext* callee_memory) NIRVANA_NOEXCEPT :
		RequestLocalAsync (proxy, op_idx, callee_memory)
	{
		caller_memory_ = callee_memory_;
	}

	virtual RqKind kind () const NIRVANA_NOEXCEPT;

};

template <class Base>
class RequestLocalImpl :
	public Base
{
public:
	RequestLocalImpl (ServantProxyBase& proxy, IOReference::OperationIndex op_idx,
		Nirvana::Core::MemContext* callee_memory) NIRVANA_NOEXCEPT :
		Base (proxy, op_idx, callee_memory)
	{
		Base::cur_ptr_ = block_;
	}

	virtual void reset () NIRVANA_NOEXCEPT
	{
		Base::reset ();
		Base::cur_ptr_ = block_;
	}

	virtual void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (!Base::ref_cnt_.decrement ()) {
			Nirvana::Core::CoreRef <Nirvana::Core::MemContext> mc =
				std::move (Base::caller_memory_);
			this->RequestLocalImpl::~RequestLocalImpl ();
			mc->heap ().release (this, sizeof (*this));
		}
	}

private:
	Octet block_ [(Base::BLOCK_SIZE - sizeof (Base))];
};

static_assert (
	sizeof (RequestLocalImpl <RequestLocal>) == RequestLocal::BLOCK_SIZE
	&&
	sizeof (RequestLocalImpl <RequestLocalAsync>) == RequestLocal::BLOCK_SIZE
	&&
	sizeof (RequestLocalImpl <RequestLocalOneway>) == RequestLocal::BLOCK_SIZE,
	"sizeof (RequestLocal)");

}
}
}

#endif
