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
namespace Core {

class ServantProxyBase;

class RequestLocal :
	public servant_traits <Internal::IORequest>::Servant <RequestLocal>,
	public Internal::LifeCycleRefCnt <RequestLocal>,
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

	Internal::IOReference::OperationIndex op_idx () const NIRVANA_NOEXCEPT
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
		check_align (align);
		marshal_op ();
		write (align, size, data);
	}

	/// Unmarshal CDR data.
	/// 
	/// \param align Data alignment
	/// \param size  Data size.
	/// \param data Pointer to the data buffer.
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	bool unmarshal (size_t align, size_t size, void* data)
	{
		check_align (align);
		read (align, size, data);
		return false;
	}

	/// Marshal CDR sequence.
	/// 
	/// \param align Data alignment
	/// \param element_size Element size.
	/// \param element_count Count of elements.
	/// \param data Pointer to the data with common-data-representation (CDR).
	/// \param allocated_size If this parameter is not zero, the request may adopt the memory block.
	///   If request adopts the memory block, it sets \p allocated_size to 0.
	void marshal_seq (size_t align, size_t element_size,
		size_t element_count, void* data, size_t& allocated_size)
	{
		check_align (align);
		marshal_op ();

		write (alignof (size_t), sizeof (size_t), &element_count);
		if (element_count)
			marshal_segment (align, element_size, element_count, data, allocated_size);
	}

	/// Unmarshal CDR sequence.
	/// 
	/// \param align Data alignment
	/// \param element_size Element size.
	/// \param [out] element_count Count of elements.
	/// \param [out] data Pointer to the allocated memory block with common-data-representation (CDR).
	///                   The caller becomes an owner of this memory block.
	/// \param [out] allocated_size Size of the allocated memory block.
	///              
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	bool unmarshal_seq (size_t align, size_t element_size,
		size_t& element_count, void*& data, size_t& allocated_size)
	{
		check_align (align);
		read (alignof (size_t), sizeof (size_t), &element_count);
		if (element_count)
			unmarshal_segment (data, allocated_size);
		else {
			data = nullptr;
			allocated_size = 0;
		}
		return false;
	}

	///@}

	///@{
	/// Marshal/unmarshal sequences.

	/// Write marshaling sequence size.
	/// 
	/// \param element_count The sequence size.
	void marshal_seq_begin (size_t element_count)
	{
		marshal_op ();
		write (alignof (size_t), sizeof (size_t), &element_count);
	}

	/// Get unmarshalling sequence size.
	/// 
	/// \returns The sequence size.
	size_t unmarshal_seq_begin ()
	{
		size_t element_count;
		read (alignof (size_t), sizeof (size_t), &element_count);
		return element_count;
	}

	///@}

	///@{
	/// Marshal/unmarshal character data.

	template <typename C>
	void marshal_char (size_t count, const C* data)
	{
		marshal_op ();
		write (alignof (C), count * sizeof (C), data);
	}

	template <typename C>
	void unmarshal_char (size_t count, C* data)
	{
		read (alignof (C), count * sizeof (C), data);
	}

	template <typename C>
	void marshal_string (Internal::StringT <C>& s, bool move)
	{
		marshal_op ();

		typedef typename Internal::Type <Internal::StringT <C> >::ABI ABI;
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
		write (alignof (size_t), sizeof (size_t), &size);
		if (size <= ABI::SMALL_CAPACITY)
			write (alignof (C), size * sizeof (C), ptr);
		else {
			size_t allocated = move ? abi.allocated () : 0;
			marshal_segment (alignof (C), sizeof (C), size + 1, ptr, allocated);
			if (move && !allocated)
				abi.reset ();
		}
	}

	template <typename C>
	void unmarshal_string (Internal::StringT <C>& s)
	{
		typedef typename Internal::Type <Internal::StringT <C> >::ABI ABI;

		size_t size;
		read (alignof (size_t), sizeof (size_t), &size);
		Internal::StringT <C> tmp;
		ABI& abi = (ABI&)tmp;
		if (size <= ABI::SMALL_CAPACITY) {
			if (size) {
				abi.small_size (size);
				C* p = abi.small_pointer ();
				read (alignof (C), sizeof (C) * size, p);
				p [size] = 0;
			}
		} else {
			void* data;
			size_t allocated_size;
			unmarshal_segment (data, allocated_size);
			abi.large_size (size);
			abi.large_pointer ((C*)data);
			abi.allocated (allocated_size);
		}
		s = std::move (tmp);
	}

	template <typename C>
	void marshal_char_seq (IDL::Sequence <C>& s, bool move)
	{
		typedef typename Internal::Type <IDL::Sequence <C> >::ABI ABI;
		ABI& abi = (ABI&)s;
		if (move) {
			marshal_seq (alignof (C), sizeof (C), abi.size, abi.ptr, abi.allocated);
			if (!abi.allocated)
				abi.reset ();
		} else {
			size_t zero = 0;
			marshal_seq (alignof (C), sizeof (C), abi.size, abi.ptr, zero);
		}
	}

	template <typename C>
	void unmarshal_char_seq (IDL::Sequence <C>& s)
	{
		typedef typename Internal::Type <IDL::Sequence <C> >::ABI ABI;
		IDL::Sequence <C> tmp;
		ABI& abi = (ABI&)tmp;
		unmarshal_seq (alignof (C), sizeof (C), abi.size, (void*&)abi.ptr, abi.allocated);
		s = std::move (tmp);
	}

	void marshal_wchar (size_t count, const WChar* data)
	{
		marshal_char (count, data);
	}

	void unmarshal_wchar (size_t count, WChar* data)
	{
		unmarshal_char (count, data);
	}

	void marshal_wstring (IDL::WString& s, bool move)
	{
		marshal_string (s, move);
	}

	void unmarshal_wstring (IDL::WString& s)
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
	Interface::_ref_type unmarshal_interface (const IDL::String& interface_id);

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
		return unmarshal_interface (Internal::RepIdOf <TypeCode>::id).template downcast <TypeCode> ();
	}

	/// Marshal value type.
	/// 
	/// \param val  ValueBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_value (Internal::Interface::_ptr_type val, bool output)
	{
		if (output || !val || caller_memory_ == callee_memory_)
			marshal_interface (val);
		else
			marshal_value_copy (value_type2base (val), val->_epv ().interface_id);
	}

	void marshal_value_copy (ValueBase::_ptr_type base, const IDL::String& interface_id);

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// 
	/// \returns Value type interface.
	Interface::_ref_type unmarshal_value (const IDL::String& interface_id)
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
	Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id)
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
		Internal::Type <Any>::marshal_out (e, _get_ptr ());
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

	RequestLocal (ServantProxyBase& proxy, Internal::IOReference::OperationIndex op_idx,
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
	void write (size_t align, size_t size, const void* data);
	void read (size_t align, size_t size, void* data);
	void marshal_segment (size_t align, size_t element_size,
		size_t element_count, void* data, size_t& allocated_size);
	void unmarshal_segment (void*& data, size_t& allocated_size);

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

protected:
	Nirvana::Core::RefCounter ref_cnt_;
	CoreRef <MemContext> caller_memory_;
	CoreRef <MemContext> callee_memory_;
	Octet* cur_ptr_;

private:
	struct BlockHdr
	{
		BlockHdr* next;
		size_t size;
	};

	struct Element
	{
		void* ptr; // Can not be null
		Element* next;
	};

	struct Segment : Element
	{
		size_t allocated_size;
	};

	typedef Element ItfRecord;

	Element* get_element_buffer (size_t size);

	void allocate_block (size_t align, size_t size);
	void next_block ();

	void invert_list (Element*& head);

private:
	CoreRef <ServantProxyBase> proxy_;
	Internal::IOReference::OperationIndex op_idx_;
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
	RequestLocalAsync (ServantProxyBase& proxy, Internal::IOReference::OperationIndex op_idx,
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
	RequestLocalOneway (ServantProxyBase& proxy, Internal::IOReference::OperationIndex op_idx,
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
	RequestLocalImpl (ServantProxyBase& proxy, Internal::IOReference::OperationIndex op_idx,
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

#endif
