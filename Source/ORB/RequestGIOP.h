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
#include "StreamIn.h"
#include "StreamOut.h"
#include <CORBA/IORequest_s.h>
#include "RqHelper.h"
#include "../LifeCyclePseudo.h"
#include "GIOP.h"
#include "CodeSetConverter.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for GIOP messages
class NIRVANA_NOVTABLE RequestGIOP :
	public servant_traits <Internal::IORequest>::Servant <RequestGIOP>,
	protected RqHelper,
	public Internal::LifeCycleRefCnt <RequestGIOP>
{
	DECLARE_CORE_INTERFACE
	friend class Internal::LifeCycleRefCnt <RequestGIOP>;

public:
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
			stream_out_->write (align, size, data);
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
		stream_in_->read (align, size, data);
		return stream_in_->other_endian ();
	}

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
		if (!marshal_op ())
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
	bool unmarshal_seq (size_t align, size_t element_size, size_t& element_count, void*& data,
		size_t& allocated_size)
	{
		check_align (align);
		return stream_in_->read_seq (align, element_size, element_count, data, allocated_size);
	}

	///@}

	///@{
	/// Marshal/unmarshal sequences.

	/// Write marshaling sequence size.
	/// 
	/// \param element_count The sequence size.
	bool marshal_seq_begin (size_t element_count)
	{
		if (!marshal_op ())
			return false;
		stream_out_->write_size (element_count);
		return true;
	}

	/// Get unmarshaling sequence size.
	/// 
	/// \returns The sequence size.
	size_t unmarshal_seq_begin ()
	{
		return stream_in_->read_size ();
	}

	///@}

	///@{
	/// Marshal/unmarshal character data.

	void marshal_char (size_t count, const Char* data)
	{
		if (marshal_op ())
			stream_out_->write (alignof (Char), count * sizeof (Char), data);
	}

	void unmarshal_char (size_t count, Char* data)
	{
		stream_in_->read (alignof (Char), count * sizeof (Char), data);
	}

	void marshal_string (IDL::String& s, bool move)
	{
		if (marshal_op ())
			code_set_conv_->marshal_string (s, move, *stream_out_);
	}

	void unmarshal_string (IDL::String& s)
	{
		code_set_conv_->unmarshal_string (*stream_in_, s);
	}

	template <typename C>
	void marshal_char_seq (IDL::Sequence <C>& s, bool move)
	{
		if (marshal_op ())
			stream_out_->write_seq (s, move);
	}

	template <typename C>
	void unmarshal_char_seq (IDL::Sequence <C>& s)
	{
		stream_in_->read_seq (s);
	}

	void marshal_wchar (size_t count, const WChar* data)
	{
		code_set_conv_w_->marshal_char (count, data, *stream_out_);
	}

	void unmarshal_wchar (size_t count, WChar* data)
	{
		code_set_conv_w_->unmarshal_char (*stream_in_, count, data);
	}

	void marshal_wstring (IDL::WString& s, bool move)
	{
		code_set_conv_w_->marshal_string (s, move, *stream_out_);
	}

	void unmarshal_wstring (IDL::WString& s)
	{
		code_set_conv_w_->unmarshal_string (*stream_in_, s);
	}

	void marshal_wchar_seq (WCharSeq& s, bool move)
	{
		code_set_conv_w_->marshal_char_seq (s, move, *stream_out_);
	}

	void unmarshal_wchar_seq (WCharSeq& s)
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
		throw NO_IMPLEMENT ();
	}

	/// Unmarshal interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	Internal::Interface::_ref_type unmarshal_interface (const IDL::String& interface_id)
	{
		throw NO_IMPLEMENT ();
	}

	/// Marshal TypeCode.
	/// 
	/// \param tc TypeCode.
	void marshal_type_code (TypeCode::_ptr_type tc)
	{
		throw NO_IMPLEMENT ();
	}

	/// Unmarshal TypeCode.
	/// 
	/// \returns TypeCode.
	TypeCode::_ref_type unmarshal_type_code ()
	{
		throw NO_IMPLEMENT ();
	}

	/// Marshal value type.
	/// 
	/// \param val  ValueBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_value (Internal::Interface::_ptr_type val, bool output)
	{
		throw NO_IMPLEMENT ();
	}

	/// Unmarshal value type.
	/// 
	/// \param rep_id The value type repository id.
	/// 
	/// \returns Value type interface.
	Internal::Interface::_ref_type unmarshal_value (const IDL::String& interface_id)
	{
		throw NO_IMPLEMENT ();
	}

	/// Marshal abstract interface.
	/// 
	/// \param itf The interface derived from AbstractBase.
	/// \param output Output parameter marshaling. Haven't to perform deep copy.
	void marshal_abstract (Internal::Interface::_ptr_type itf, bool output)
	{
		throw NO_IMPLEMENT ();
	}

	/// Unmarshal abstract interface.
	/// 
	/// \param rep_id The interface repository id.
	/// 
	/// \returns Interface.
	Internal::Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id)
	{
		throw NO_IMPLEMENT ();
	}

	///@}

	/// \returns Response flags.
	UShort response_flags () const NIRVANA_NOEXCEPT
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

	/// Check if the request is completed with an exception.
	virtual bool is_exception () const NIRVANA_NOEXCEPT = 0;

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

	virtual ~RequestGIOP ()
	{}

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

	void code_set_converter (Nirvana::Core::CoreRef <CodeSetConverter>&& csc) NIRVANA_NOEXCEPT
	{
		code_set_conv_ = std::move (csc);
	}

	void code_set_converter (Nirvana::Core::CoreRef <CodeSetConverterW>&& csc) NIRVANA_NOEXCEPT
	{
		code_set_conv_w_ = std::move (csc);
	}

	///@}

protected:
	RequestGIOP (unsigned GIOP_minor, bool client_side);

	RequestGIOP (const RequestGIOP& src) :
		code_set_conv_ (src.code_set_conv_),
		code_set_conv_w_ (src.code_set_conv_w_)
	{}

	virtual bool marshal_op () = 0;

	/// Set the size of message in the output message header.
	/// Must be called for remote messages.
	/// In the ESIOP we do not use the message size to allow > 4GB data transferring.
	void set_out_size ();

protected:
	unsigned response_flags_;
	Nirvana::Core::CoreRef <StreamIn> stream_in_;
	Nirvana::Core::CoreRef <StreamOut> stream_out_;

private:
	Nirvana::Core::CoreRef <CodeSetConverter> code_set_conv_;
	Nirvana::Core::CoreRef <CodeSetConverterW> code_set_conv_w_;
};

}
}

#endif