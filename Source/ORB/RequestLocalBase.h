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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCALBASE_H_
#define NIRVANA_ORB_CORE_REQUESTLOCALBASE_H_
#pragma once

#include "../MemContext.h"
#include "../UserObject.h"
#include <CORBA/Server.h>
#include <CORBA/IORequest_s.h>
#include "RqHelper.h"
#include "IndirectMap.h"

namespace CORBA {
namespace Core {

/// Root base of local requests
class NIRVANA_NOVTABLE RequestLocalBase :
	public servant_traits <Internal::IORequest>::Servant <RequestLocalBase>,
	public Internal::LifeCycleRefCnt <RequestLocalBase>,
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
		if (CDR_element_size > element_size || element_size % align)
			throw BAD_PARAM ();
		if (marshal_op ()) {
			write_size (element_count);
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
		if (CDR_element_size > element_size || element_size % align)
			throw BAD_PARAM ();
		size_t count = read_size ();
		if (count) {
			void* p;
			size_t size;
			unmarshal_segment (count * element_size, p, size);
			size_t allocated = allocated_size;
			if (allocated)
				Nirvana::Core::Heap::user_heap ().release (data, allocated);
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
			write_size (element_count);
			return true;
		} else
			return false;
	}

	/// Get unmarshalling sequence size.
	/// 
	/// \returns The sequence size.
	size_t unmarshal_seq_begin ()
	{
		return read_size ();
	}

	///@}

	///@{
	/// Marshal/unmarshal character data.

	template <typename C>
	void marshal_string (Internal::StringT <C>& s, bool move)
	{
		if (marshal_op ()) {
			typedef typename IDL::Type <Internal::StringT <C> >::ABI ABI;
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
			write_size (size);
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
		typedef typename IDL::Type <Internal::StringT <C> >::ABI ABI;

		size_t size = read_size ();
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
			unmarshal_segment ((size + 1) * sizeof (C), data, allocated_size);
			abi.large_size (size);
			abi.large_pointer ((C*)data);
			abi.allocated (allocated_size);
		}
		s = std::move (tmp);
	}

	void marshal_wchar (size_t count, const wchar_t* data)
	{
		if (marshal_op ())
			write (alignof (wchar_t), count * sizeof (wchar_t), data);
	}

	void unmarshal_wchar (size_t count, wchar_t* data)
	{
		read (alignof (wchar_t), count * sizeof (wchar_t), data);
	}

	void marshal_wstring (IDL::WString& s, bool move)
	{
		marshal_string (s, move);
	}

	void unmarshal_wstring (IDL::WString& s)
	{
		unmarshal_string (s);
	}

	void marshal_wchar_seq (IDL::Sequence <wchar_t>& s, bool move)
	{
		typedef typename IDL::Type <IDL::Sequence <wchar_t> >::ABI ABI;
		ABI& abi = (ABI&)s;
		if (move) {
			marshal_seq (alignof (wchar_t), sizeof (wchar_t), sizeof (wchar_t), abi.size, abi.ptr, abi.allocated);
			if (!abi.allocated)
				abi.reset ();
		} else {
			size_t zero = 0;
			marshal_seq (alignof (wchar_t), sizeof (wchar_t), sizeof (wchar_t), abi.size, abi.ptr, zero);
		}
	}

	void unmarshal_wchar_seq (IDL::Sequence <wchar_t>& s)
	{
		typedef typename IDL::Type <IDL::Sequence <wchar_t> >::ABI ABI;
		ABI& abi = (ABI&)s;
		unmarshal_seq (alignof (wchar_t), sizeof (wchar_t), sizeof (wchar_t), abi.size, (void*&)abi.ptr, abi.allocated);
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
	/// \param [out] itf The interface.
	/// 
	void unmarshal_interface (const IDL::String& interface_id, Internal::Interface::_ref_type& itf);

	/// Marshal TypeCode.
	/// 
	/// \param tc TypeCode.
	void marshal_type_code (TypeCode::_ptr_type tc)
	{
		marshal_interface (tc);
	}

	/// Unmarshal TypeCode.
	/// 
	/// \param [out] TypeCode.
	/// 
	void unmarshal_type_code (TypeCode::_ref_type& tc)
	{
		return unmarshal_interface (Internal::RepIdOf <TypeCode>::id, reinterpret_cast <Internal::Interface::_ref_type&> (tc));
	}

	/// Marshal value type.
	/// 
	/// \param val The value interface.
	void marshal_value (Internal::Interface::_ptr_type val)
	{
		if (marshal_op ()) {
			if (!val)
				write8 (0);
			else
				marshal_value_internal (value_type2base (val));
		}
	}

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// \param [out] val The value interface.
	/// 
	void unmarshal_value (const IDL::String& interface_id, Internal::Interface::_ref_type& val)
	{
		Internal::Interface::_ref_type ret;
		uintptr_t pos = (uintptr_t)cur_ptr_;
		unsigned tag = read8 ();
		switch (tag) {
		case 0:
			break;

		case 1: {
			ValueFactoryBase::_ref_type factory;
			unmarshal_interface (Internal::RepIdOf <ValueFactoryBase>::id, reinterpret_cast <Interface::_ref_type&> (factory));
			if (!factory)
				throw MARSHAL (MAKE_OMG_MINOR (1)); // Unable to locate value factory.
			ValueBase::_ref_type base (factory->create_for_unmarshal ());
			base->_unmarshal (_get_ptr ());
			value_map_.unmarshal_map ().emplace (pos, &ValueBase::_ptr_type (base));
			val = base->_query_valuetype (interface_id);
		} break;

		case 2: {
			uintptr_t p;
			read (alignof (uintptr_t), sizeof (uintptr_t), &p);
			const auto& unmarshal_map = value_map_.unmarshal_map ();
			Interface* vb = unmarshal_map.find (p);
			if (!vb)
				throw MARSHAL (); // Unexpected
			val = static_cast <ValueBase*> (vb)->_query_valuetype (interface_id);
		} break;

		default:
			throw MARSHAL (); // Unexpected
		}

		if (tag && !val)
			throw MARSHAL (); // Unexpected
	}

	/// Marshal abstract interface.
	/// 
	/// \param itf The interface derived from AbstractBase.
	void marshal_abstract (Internal::Interface::_ptr_type itf)
	{
		if (marshal_op ()) {
			if (!itf) {
				write8 (0);
				write8 (0);
			} else {
				// Downcast to AbstractBase
				AbstractBase::_ptr_type base = abstract_interface2base (itf);
				if (base->_to_object ()) {
					write8 (1);
					marshal_interface_internal (itf);
				} else {
					ValueBase::_ref_type value = base->_to_value ();
					if (!value)
						Nirvana::throw_MARSHAL (); // Unexpected
					write8 (0);
					marshal_value_internal (value);
				}
			}
		}
	}

	/// Unmarshal abstract interface.
	/// 
	/// \param rep_id The interface repository id.
	/// \param [out] itf The interface.
	/// 
	void unmarshal_abstract (const IDL::String& interface_id, Internal::Interface::_ref_type& itf)
	{
		unsigned tag = read8 ();
		if (tag)
			unmarshal_interface (interface_id, itf);
		else
			unmarshal_value (interface_id, itf);
	}

	///@}

	///@{
	/// Callee operations.

	/// End of the data unmarshaling.
	/// 
	/// Marshaling resources may be released.
	void unmarshal_end () noexcept
	{
		clear_on_callee_side ();

		// callee_memory_ may contain temporary memory context.
		// Release it. It will be assigned to callee memory context in marshal_op().
		callee_memory_ = nullptr;
	}

	/// Return exception to caller.
	/// Operation has move semantics so \p e may be cleared.
	void set_exception (Any& e);

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
			IDL::Type <Any>::unmarshal (_get_ptr (), e);
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

	void set_exception (Exception&& e) noexcept;
	void set_unknown_exception () noexcept;

protected:
	enum class State : uint8_t
	{
		CALLER,
		CALL,
		CALLEE,
		SUCCESS,
		EXCEPTION
	};

	RequestLocalBase (Nirvana::Core::Heap* callee_memory, unsigned response_flags)
		noexcept;

	~RequestLocalBase ()
	{
		cleanup ();
	}

	Nirvana::Core::Heap& target_memory ();
	Nirvana::Core::Heap& source_memory ();

	Nirvana::Core::Heap* target_heap () const noexcept
	{
		return callee_memory_;
	}

	const Octet* cur_block_end () const noexcept
	{
		if (!cur_block_)
			return (const Octet*)this + BLOCK_SIZE;
		else
			return (const Octet*)cur_block_ + cur_block_->size;
	}

	bool marshal_op () noexcept;
	void write (size_t align, size_t size, const void* data);

	void write8 (unsigned val);

	void write_size (size_t size);

	void read (size_t align, size_t size, void* data);

	unsigned read8 ();

	size_t read_size ();

	void marshal_interface_internal (Internal::Interface::_ptr_type itf);

	void marshal_segment (size_t align, size_t element_size,
		size_t element_count, void* data, size_t& allocated_size);
	void unmarshal_segment (size_t min_size, void*& data, size_t& allocated_size);

	void rewind () noexcept;

	virtual void reset () noexcept
	{
		cur_block_ = nullptr;
		value_map_.clear ();
	}

	void clear_on_callee_side () noexcept;

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

	Octet* allocate_block (size_t align, size_t size);
	const Octet* next_block (size_t align);

	void invert_list (Element*& head);

	void marshal_value_internal (ValueBase::_ptr_type base);

protected:
	Nirvana::Core::RefCounter ref_cnt_;
	servant_reference <Nirvana::Core::MemContext> caller_memory_;
	servant_reference <Nirvana::Core::Heap> callee_memory_;
	Octet* cur_ptr_;
	State state_;
	uint8_t response_flags_;
	std::atomic <bool> cancelled_;

private:
	BlockHdr* first_block_;
	BlockHdr* cur_block_;
	ItfRecord* interfaces_;
	Segment* segments_;
	IndirectMapItf value_map_;
};

template <class Base>
class RequestLocalImpl :
	public Base
{
public:
	template <class ... Args>
	RequestLocalImpl (Args&& ... args) noexcept :
		Base (std::forward <Args> (args)...)
	{
		Base::cur_ptr_ = block_;
		static_assert (sizeof (RequestLocalImpl <Base>) == RequestLocalBase::BLOCK_SIZE, "sizeof (RequestLocal)");
	}

	virtual void reset () noexcept override
	{
		Base::reset ();
		Base::cur_ptr_ = block_;
	}

	virtual void _remove_ref () noexcept override
	{
		if (0 == Base::ref_cnt_.decrement ()) {
			servant_reference <Nirvana::Core::MemContext> mc = Base::caller_memory_;
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
