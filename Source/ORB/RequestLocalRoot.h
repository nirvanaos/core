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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCALROOT_H_
#define NIRVANA_ORB_CORE_REQUESTLOCALROOT_H_
#pragma once

#include "../MemContext.h"
#include "../UserObject.h"
#include <CORBA/Server.h>
#include <CORBA/IORequest_s.h>
#include "RqHelper.h"

namespace CORBA {
namespace Core {

/// Root base of local requests
class NIRVANA_NOVTABLE RequestLocalRoot :
	public servant_traits <Internal::IORequest>::Servant <RequestLocalRoot>,
	public Internal::LifeCycleRefCnt <RequestLocalRoot>,
	public Nirvana::Core::UserObject,
	protected RqHelper
{
	/// A MARSHAL exception with minor code 9 indicates that fewer bytes were present in a message
	/// than indicated by	the count. This condition can arise if the sender sends a message
	/// in fragments, and the receiver detects that the final fragment was received but contained
	/// insufficient data for all parameters to be unmarshaled.
	static const uint32_t MARSHAL_MINOR_FEWER = MAKE_OMG_MINOR (9);

public:
	static const size_t BLOCK_SIZE = (32 * sizeof (size_t)
		+ alignof (max_align_t) - 1) / alignof (max_align_t) * alignof (max_align_t);

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	virtual void _remove_ref () noexcept = 0;

	Nirvana::Core::MemContext* memory () const noexcept
	{
		return callee_memory_;
	}

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
		if (marshal_op ())
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
	/// \param align CDR data alignment.
	///   For the constructed types is an alignment of the first primitive type.
	/// 
	/// \param element_size Element size in memory.
	/// 
	/// \param CDR_element_size CDR element size.
	///   CDR_element_size <= element_size.
	/// 
	/// \param element_count Count of elements.
	/// 
	/// \param data Pointer to the data with common-data-representation (CDR).
	/// 
	/// \param allocated_size If this parameter is not zero, the request may adopt the memory block.
	///   When request adopts the memory block, it sets \p allocated_size to 0.
	/// 
	void marshal_seq (size_t align, size_t element_size, size_t CDR_element_size, size_t element_count, void* data,
		size_t& allocated_size)
	{
		check_align (align);
		if (CDR_element_size > element_size)
			throw BAD_PARAM ();
		if (marshal_op ()) {
			write (alignof (size_t), sizeof (size_t), &element_count);
			if (element_count)
				marshal_segment (align, element_size, element_count, data, allocated_size);
		}
	}

	/// Unmarshal CDR sequence.
	/// 
	/// \param align Data alignment.
	/// 
	/// \param element_size Element size in memory.
	/// 
	/// \param CDR_element_size Last element size.
	///   Last element may be shorter due to the data alignment.
	/// 
	/// \param [out] element_count Count of elements.
	/// 
	/// \param [inout] data Pointer to the allocated memory block with common-data-representation (CDR).
	///   The caller becomes an owner of this memory block.
	/// 
	/// \param [inout] allocated_size Size of the allocated memory block.
	///              
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	/// 
	bool unmarshal_seq (size_t align, size_t element_size, size_t CDR_element_size, size_t& element_count, void*& data,
		size_t& allocated_size)
	{
		check_align (align);
		if (CDR_element_size > element_size)
			throw BAD_PARAM ();
		size_t count;
		read (alignof (size_t), sizeof (size_t), &count);
		if (count) {
			void* p;
			size_t size;
			unmarshal_segment (p, size);
			size_t allocated = allocated_size;
			if (allocated)
				Nirvana::Core::MemContext::current ().heap ().release (data, allocated);
			allocated_size = size;
			data = p;
		}
		element_count = count;
		return false;
	}

	///@}

	///@{
	/// Marshal/unmarshal sequences.

	/// Write marshaling sequence size.
	/// 
	/// \param element_count The sequence size.
	/// \returns `false` to skip marshaling.
	bool marshal_seq_begin (size_t element_count)
	{
		if (marshal_op ()) {
			write (alignof (size_t), sizeof (size_t), &element_count);
			return true;
		} else
			return false;
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
		if (marshal_op ())
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
		if (marshal_op ()) {
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
			if (size) {
				if (size <= ABI::SMALL_CAPACITY)
					write (alignof (C), size * sizeof (C), ptr);
				else {
					size_t allocated = move ? abi.allocated () : 0;
					marshal_segment (alignof (C), sizeof (C), size + 1, ptr, allocated);
					if (move && !allocated)
						abi.reset ();
				}
			}
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
			marshal_seq (alignof (C), sizeof (C), sizeof (C), abi.size, abi.ptr, abi.allocated);
			if (!abi.allocated)
				abi.reset ();
		} else {
			size_t zero = 0;
			marshal_seq (alignof (C), sizeof (C), sizeof (C), abi.size, abi.ptr, zero);
		}
	}

	template <typename C>
	void unmarshal_char_seq (IDL::Sequence <C>& s)
	{
		typedef typename Internal::Type <IDL::Sequence <C> >::ABI ABI;
		ABI& abi = (ABI&)s;
		unmarshal_seq (alignof (C), sizeof (C), sizeof (C), abi.size, (void*&)abi.ptr, abi.allocated);
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
	void marshal_interface (Internal::Interface::_ptr_type itf);

	/// Unmarshal interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	Internal::Interface::_ref_type unmarshal_interface (const IDL::String& interface_id);

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
	virtual void marshal_value (Internal::Interface::_ptr_type val);

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// 
	/// \returns Value type interface.
	virtual Internal::Interface::_ref_type unmarshal_value (const IDL::String& interface_id);

	/// Marshal abstract interface.
	/// 
	/// \param itf The interface derived from AbstractBase.
	virtual void marshal_abstract (Internal::Interface::_ptr_type itf);

	/// Unmarshal abstract interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	virtual Internal::Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id);

	///@}

	///@{
	/// Callee operations.

	/// End of input data marshaling.
	/// 
	/// Marshaling resources may be released.
	void unmarshal_end () noexcept
	{
		clear ();
		state_ = State::CALLEE;
	}

	/// Return exception to caller.
	/// Operation has move semantics so \p e may be cleared.
	void set_exception (Any& e)
	{
		if (e.type ()->kind () != TCKind::tk_except)
			throw BAD_PARAM (MAKE_OMG_MINOR (21));
		clear ();
		state_ = State::EXCEPTION;
		if (response_flags_) {
			try {
				Internal::Type <Any>::marshal_out (e, _get_ptr ());
			} catch (...) {}
			rewind ();
		}
		finalize ();
	}

	/// Marks request as successful.
	void success ()
	{
		rewind ();
		state_ = State::SUCCESS;
		finalize ();
	}

	///@}

	///@{
	/// Caller operations.

	/// Invoke request.
	/// 
	virtual void invoke ();

	/// Unmarshal exception.
	/// \param [out] e If request completed with exception, receives the exception.
	///                Otherwise unchanged.
	/// \returns `true` if e contains an exception.
	bool get_exception (Any& e)
	{
		if (State::EXCEPTION == state_) {
			Internal::Type <Any>::unmarshal (_get_ptr (), e);
			return true;
		} else
			return false;
	}

	///@}

	///@{

	/// Cancel the request.
	virtual void cancel ()
	{
		Nirvana::throw_BAD_OPERATION ();
	}

	///@}

	unsigned response_flags () const noexcept
	{
		return response_flags_;
	}

protected:
	using MemContext = Nirvana::Core::MemContext;

	enum class State : uint8_t
	{
		CALLER,
		CALL,
		CALLEE,
		SUCCESS,
		EXCEPTION
	};

	RequestLocalRoot (Nirvana::Core::MemContext* callee_memory, unsigned response_flags)
		noexcept;

	Nirvana::Core::MemContext& target_memory ();
	Nirvana::Core::MemContext& source_memory ();

	const Octet* cur_block_end () const noexcept
	{
		if (!cur_block_)
			return (const Octet*)this + BLOCK_SIZE;
		else
			return (const Octet*)cur_block_ + cur_block_->size;
	}

	bool marshal_op () noexcept;
	void write (size_t align, size_t size, const void* data);

	void write32 (uint32_t val)
	{
		write (4, 4, &val);
	}

	void write8 (uint8_t val)
	{
		write (1, 1, &val);
	}

	void read (size_t align, size_t size, void* data);

	uint32_t read32 ()
	{
		uint32_t val;
		read (4, 4, &val);
		return val;
	}

	uint8_t read8 ()
	{
		uint8_t val;
		read (1, 1, &val);
		return val;
	}

	void marshal_interface_internal (Internal::Interface::_ptr_type itf);

	void marshal_segment (size_t align, size_t element_size,
		size_t element_count, void* data, size_t& allocated_size);
	void unmarshal_segment (void*& data, size_t& allocated_size);

	void rewind () noexcept;

	virtual void reset () noexcept
	{
		cur_block_ = nullptr;
	}

	void clear () noexcept
	{
		cleanup ();
		reset ();
	}

	void cleanup () noexcept;

	State state () const noexcept
	{
		return state_;
	}

	bool is_cancelled () const noexcept
	{
		return cancelled_.load (std::memory_order_acquire);
	}

	bool set_cancelled () noexcept
	{
		return response_flags () && !cancelled_.exchange (true, std::memory_order_release);
	}

	virtual void finalize () noexcept
	{}

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

protected:
	Nirvana::Core::RefCounter ref_cnt_;
	servant_reference <MemContext> caller_memory_;
	servant_reference <MemContext> callee_memory_;
	Octet* cur_ptr_;
	State state_;
	uint8_t response_flags_;
	std::atomic <bool> cancelled_;

private:
	BlockHdr* first_block_;
	BlockHdr* cur_block_;
	ItfRecord* interfaces_;
	Segment* segments_;
};

template <class Base>
class RequestLocalImpl :
	public Base
{
public:
	template <class ... Args>
	RequestLocalImpl (Args ... args) noexcept :
		Base (std::forward <Args> (args)...)
	{
		Base::cur_ptr_ = block_;
		static_assert (sizeof (RequestLocalImpl <Base>) == RequestLocalRoot::BLOCK_SIZE, "sizeof (RequestLocal)");
	}

	virtual void reset () noexcept override
	{
		Base::reset ();
		Base::cur_ptr_ = block_;
	}

	virtual void _remove_ref () noexcept override
	{
		if (0 == Base::ref_cnt_.decrement ()) {
			servant_reference <Nirvana::Core::MemContext> mc =
				std::move (Base::caller_memory_);
			this->RequestLocalImpl::~RequestLocalImpl ();
			mc->heap ().release (this, sizeof (*this));
		}
	}

private:
	Octet block_ [(Base::BLOCK_SIZE - sizeof (Base))];
};

}
}

#endif
