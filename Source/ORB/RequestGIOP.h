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
#ifndef NIRVANA_ORB_CORE_REQUESTGIOP_H_
#define NIRVANA_ORB_CORE_REQUESTGIOP_H_
#pragma once

#include <CORBA/Server.h>
#include "StreamInEncap.h"
#include "StreamOutEncap.h"
#include <CORBA/IORequest_s.h>
#include "RqHelper.h"
#include "../LifeCyclePseudo.h"
#include <CORBA/IIOP.h>
#include "CodeSetConverter.h"
#include "POA_Root.h"
#include "LocalAddress.h"
#include "IndirectMapUnmarshal.h"
#include "Domain.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for GIOP messages
class NIRVANA_NOVTABLE RequestGIOP :
	public servant_traits <Internal::IORequest>::Servant <RequestGIOP>,
	protected RqHelper,
	public Internal::LifeCycleRefCnt <RequestGIOP>,
	public Nirvana::Core::UserObject
{
	static const size_t CHUNK_SIZE_LIMIT = 0x10000;

protected:
	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT;

	friend class Internal::LifeCycleRefCnt <RequestGIOP>;

	template <class T>
	friend class CORBA::servant_reference;

	template <class T>
	friend class Nirvana::Core::Ref;

public:
	/// When request object is created, it saves the current memory context reference.
	///
	Nirvana::Core::MemContext& memory () const NIRVANA_NOEXCEPT
	{
		return *mem_context_;
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
		if (marshal_chunk ())
			stream_out_->write_c (align, size, data);
	}

	/// Unmarshal CDR data.
	/// 
	/// \param align Data alignment
	/// \param size  Data size.
	/// \param data Pointer to the data buffer.
	/// \returns `true` if the byte order must be swapped after unmarshaling.
	virtual bool unmarshal (size_t align, size_t size, void* data);

	/// Marshal CDR sequence.
	/// 
	/// \param align Data alignment
	/// \param element_size Element size.
	/// \param element_count Count of elements.
	/// \param data Pointer to the data with common-data-representation (CDR).
	/// \param allocated_size If this parameter is not zero, the request may adopt the memory block.
	///   If request adopts the memory block, it sets \p allocated_size to 0.
	void marshal_seq (size_t align, size_t element_size, size_t element_count, void* data,
		size_t& allocated_size)
	{
		check_align (align);
		if (!marshal_chunk ())
			return;

		stream_out_->write_seq (align, element_size, element_count, data, allocated_size);
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
	virtual bool unmarshal_seq (size_t align, size_t element_size, size_t& element_count, void*& data,
		size_t& allocated_size);

	///@}

	///@{
	/// Marshal/unmarshal sequences.

	/// Write marshaling sequence size.
	/// 
	/// \param element_count The sequence size.
	bool marshal_seq_begin (size_t element_count)
	{
		if (!marshal_chunk ())
			return false;
		stream_out_->write_size (element_count);
		return true;
	}

	/// Get unmarshaling sequence size.
	/// 
	/// \returns The sequence size.
	virtual size_t unmarshal_seq_begin ()
	{
		return stream_in_->read_size ();
	}

	///@}

	///@{
	/// Marshal/unmarshal character data.

	void marshal_char (size_t count, const Char* data)
	{
		if (marshal_chunk ())
			stream_out_->write_c (alignof (Char), count * sizeof (Char), data);
	}

	virtual void unmarshal_char (size_t count, Char* data)
	{
		stream_in_->read (alignof (Char), count * sizeof (Char), data);
	}

	void marshal_string (IDL::String& s, bool move)
	{
		if (marshal_chunk ())
			code_set_conv_->marshal_string (s, move, *stream_out_);
	}

	virtual void unmarshal_string (IDL::String& s)
	{
		code_set_conv_->unmarshal_string (*stream_in_, s);
	}

	template <typename C>
	void marshal_char_seq (IDL::Sequence <C>& s, bool move)
	{
		if (marshal_chunk ())
			stream_out_->write_seq (s, move);
	}

	virtual void unmarshal_char_seq (IDL::Sequence <Char>& s)
	{
		stream_in_->read_seq (s);
	}

	void marshal_wchar (size_t count, const WChar* data)
	{
		code_set_conv_w_->marshal_char (count, data, *stream_out_);
	}

	virtual void unmarshal_wchar (size_t count, WChar* data)
	{
		code_set_conv_w_->unmarshal_char (*stream_in_, count, data);
	}

	void marshal_wstring (IDL::WString& s, bool move)
	{
		code_set_conv_w_->marshal_string (s, move, *stream_out_);
	}

	virtual void unmarshal_wstring (IDL::WString& s)
	{
		code_set_conv_w_->unmarshal_string (*stream_in_, s);
	}

	void marshal_wchar_seq (WCharSeq& s, bool move)
	{
		code_set_conv_w_->marshal_char_seq (s, move, *stream_out_);
	}

	virtual void unmarshal_wchar_seq (WCharSeq& s)
	{
		code_set_conv_w_->unmarshal_char_seq (*stream_in_, s);
	}

	///@}

	///@{
	/// Object operations.

	/// Marshal interface.
	/// 
	/// \param itf The interface derived from Object.
	void marshal_interface (Internal::Interface::_ptr_type itf)
	{
		if (marshal_chunk ())
			marshal_object (interface2object (itf));
	}

	/// Unmarshal interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	virtual Internal::Interface::_ref_type unmarshal_interface (const IDL::String& interface_id);

private:
	typedef Nirvana::Core::MapUnorderedUnstable <void*, size_t> IndirectMapMarshal;
	static void marshal_type_code (StreamOut& stream, TypeCode::_ptr_type tc, IndirectMapMarshal& map, size_t parent_offset);
	void marshal_object (Object::_ptr_type obj);

public:
	/// Marshal TypeCode.
	/// 
	/// \param tc TypeCode.
	void marshal_type_code (TypeCode::_ptr_type tc)
	{
		if (!marshal_chunk ())
			return;

		IndirectMapMarshal map;
		marshal_type_code (*stream_out_, tc, map, 0);
	}

	/// Unmarshal TypeCode.
	/// 
	/// \returns TypeCode.
	virtual TypeCode::_ref_type unmarshal_type_code ();

	typedef Nirvana::Core::MapUnorderedUnstable <RepositoryId, size_t> IndirectRepIdMarshal;
	typedef Nirvana::Core::MapUnorderedUnstable <size_t, RepositoryId> IndirectRepIdUnmarshal;

	/// Marshal value type.
	/// 
	/// \param val  ValueBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_value (Internal::Interface::_ptr_type val, bool output);

	void marshal_value (ValueBase::_ptr_type base, Interface::_ptr_type val, bool output);

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// 
	/// \returns Value type interface.
	virtual Internal::Interface::_ref_type unmarshal_value (const IDL::String& interface_id);

	/// Marshal abstract interface.
	/// 
	/// \param itf The interface derived from AbstractBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_abstract (Internal::Interface::_ptr_type itf, bool output)
	{
		if (!marshal_chunk ())
			return;

		if (itf) {
			// Downcast to AbstractBase
			AbstractBase::_ptr_type base = abstract_interface2base (itf);
			Object::_ref_type obj = base->_to_object ();
			if (obj) {
				Boolean is_object = true;
				stream_out_->write_c (1, 1, &is_object);
				marshal_object (obj);
			} else {
				ValueBase::_ref_type value = base->_to_value ();
				if (!value)
					Nirvana::throw_MARSHAL ();
				Boolean is_object = false;
				stream_out_->write_c (1, 1, &is_object);
				marshal_value (value, itf, output);
			}
		} else {
			Boolean is_object = false;
			stream_out_->write_c (1, 1, &is_object);
			stream_out_->write32 (0);
		}
	}

	/// Unmarshal abstract interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	virtual Internal::Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id);

	///@}

	/// \returns Response flags.
	unsigned response_flags () const NIRVANA_NOEXCEPT
	{
		return response_flags_;
	}

	///@{
	/// Callee operations.

	/// End of input data marshaling.
	/// 
	/// Marshaling resources may be released.
	virtual void unmarshal_end ();

	/// Return exception to caller.
	/// Operation has move semantics so \p e may be cleared.
	virtual void set_exception (Any& e) = 0;

	/// Marks request as successful.
	virtual void success () = 0;

	///@}

	///@{
	/// Caller operations.

	/// Invoke request.
	/// 
	virtual void invoke () = 0;

	/// Unmarshal exception.
	/// \param [out] e If request completed with exception, receives the exception.
	///                Otherwise unchanged.
	/// \returns `true` if e contains an exception.
	virtual bool get_exception (Any& e) = 0;

	///@}

	///@{
	/// Asynchronous caller operations.

	/// Non-blocking check for the request completion.
	/// 
	/// \returns `true` if request is completed.
	virtual bool completed () const NIRVANA_NOEXCEPT = 0;

	/// Blocking check for the request completion.
	/// 
	/// \param timeout The wait timeout.
	/// 
	/// \returns `true` if request is completed.
	virtual bool wait (uint64_t timeout) = 0;

	/// Cancel the request.
	virtual void cancel () = 0;

	///@}

	virtual ~RequestGIOP ();

	///@{
	/// API for code set converters.

	StreamIn* stream_in () const
	{
		return stream_in_;
	}

	StreamOut* stream_out () const
	{
		return stream_out_;
	}

	void code_set_converter (Nirvana::Core::Ref <CodeSetConverter>&& csc) NIRVANA_NOEXCEPT
	{
		code_set_conv_ = std::move (csc);
	}

	void code_set_converter (Nirvana::Core::Ref <CodeSetConverterW>&& csc) NIRVANA_NOEXCEPT
	{
		code_set_conv_w_ = std::move (csc);
	}

	///@}

	Domain* target_domain () const NIRVANA_NOEXCEPT
	{
		return target_domain_;
	}

protected:
	/// Constructor
	/// 
	/// \param GIOP_minor GIOP minor version number.
	/// \param response_flags Response flags
	/// \param servant_domain Target domain pointer for outgoing request,
	///        `nullptr` for incoming request.
	RequestGIOP (unsigned GIOP_minor, unsigned response_flags, Domain* servant_domain);

	virtual bool marshal_op () = 0;

	/// Set the size of message in the output message header.
	/// Must be called for remote messages.
	/// In the ESIOP we do not use the message size to allow > 4GB data transferring.
	void set_out_size ();

	void post_send () NIRVANA_NOEXCEPT;

private:
	void marshal_rep_id (IDL::String&& id);
	const IDL::String& unmarshal_rep_id ();
	bool marshal_chunk ();

protected:
	unsigned GIOP_minor_;
	unsigned response_flags_;
	servant_reference <Domain> target_domain_;
	Nirvana::Core::Ref <StreamIn> stream_in_;
	Nirvana::Core::Ref <StreamOut> stream_out_;
	Nirvana::Core::Ref <Nirvana::Core::MemContext> mem_context_;
	ReferenceSet marshaled_DGC_references_;

private:
	Nirvana::Core::RefCounter ref_cnt_;
	Long chunk_level_;
	Nirvana::Core::Ref <CodeSetConverter> code_set_conv_;
	Nirvana::Core::Ref <CodeSetConverterW> code_set_conv_w_;
	IndirectMapUnmarshal top_level_tc_unmarshal_;
	IndirectMapMarshal value_map_marshal_;
	IndirectMapUnmarshal value_map_unmarshal_;
	IndirectRepIdMarshal rep_id_map_marshal_;
	IndirectRepIdUnmarshal rep_id_map_unmarshal_;
};

}
}

#endif
