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
#ifndef NIRVANA_ORB_CORE_CODESETCONVERTER_H_
#define NIRVANA_ORB_CORE_CODESETCONVERTER_H_
#pragma once

#include <CORBA/CORBA.h>
#include "StreamIn.h"
#include "StreamOut.h"
#include "../StaticallyAllocated.h"

namespace CORBA {
namespace Core {

template <typename C>
class CodeSetConverterBase
{
protected:
	void marshal_string (Internal::StringT <C>& s, bool move, StreamOut& out)
	{
		if (sizeof (size_t) > sizeof (uint32_t) && s.size () > std::numeric_limits <uint32_t>::max ())
			throw IMP_LIMIT ();

		typedef typename Internal::Type <Internal::StringT <C> >::ABI ABI;
		ABI& abi = (ABI&)s;
		uint32_t size;
		C* ptr;
		if (abi.is_large ()) {
			size = (uint32_t)abi.large_size ();
			ptr = abi.large_pointer ();
		} else {
			size = (uint32_t)abi.small_size ();
			ptr = abi.small_pointer ();
		}
		out.write (alignof (uint32_t), sizeof (size), &size);
		if (size <= ABI::SMALL_CAPACITY)
			out.write (alignof (C), (size + 1) * sizeof (C), ptr);
		else {
			size_t allocated = move ? abi.allocated () : 0;
			out.write (1, size + 1, ptr, allocated);
			if (move && !allocated)
				abi.reset ();
		}
	}

	void unmarshal_string (StreamIn& in, Internal::StringT <C>& s)
	{
		typedef typename Internal::Type <Internal::StringT <C> >::ABI ABI;

		uint32_t size;
		in.read (alignof (uint32_t), sizeof (size), &size);
		if (sizeof (uint32_t) > sizeof (size_t) && size > std::numeric_limits <size_t>::max ())
			throw IMP_LIMIT ();

		Internal::StringT <C> tmp;
		ABI& abi = (ABI&)tmp;
		C* p;
		if (size <= ABI::SMALL_CAPACITY) {
			if (size) {
				abi.small_size (size);
				p = abi.small_pointer ();
				in.read (alignof (C), (size + 1) * sizeof (C), p);
			}
		} else {
			size_t allocated_size = size + 1;
			void* data = in.read (1, allocated_size);
			abi.large_size (size);
			abi.large_pointer (p = (C*)data);
			abi.allocated (allocated_size);
		}

		s = std::move (tmp);
	}
};

/// Code set converter for the narrow characters.
/// Note that we convert only strings.
/// Single characters always must be ASCII.
class NIRVANA_NOVTABLE CodeSetConverter :
	public CodeSetConverterBase <Char>
{
	DECLARE_CORE_INTERFACE

	typedef CodeSetConverterBase <Char> Base;
public:
	/// Marshal string.
	/// 
	/// Default implementation does not perform any conversion.
	/// 
	/// \param [in, out] s The string.
	/// \param move `true` for move semantic.
	/// \param out The output stream.
	virtual void marshal_string (IDL::String& s, bool move, StreamOut& out);

	/// Unmarshal string.
	/// 
	/// Default implementation does not perform any conversion.
	/// 
	/// \param in The input stream.
	/// \param [out] s The string.
	virtual void unmarshal_string (StreamIn& in, IDL::String& s);

	static CodeSetConverter& get_default () NIRVANA_NOEXCEPT
	{
		return default_;
	}

	static void initialize ()
	{
		default_.construct ();
	}

	static void terminate () NIRVANA_NOEXCEPT
	{}

private:
	static Nirvana::Core::StaticallyAllocated <Nirvana::Core::ImplStatic <CodeSetConverter> > default_;
};

class Request;

/// Code set converter for the wide characters.
class NIRVANA_NOVTABLE CodeSetConverterW :
	public CodeSetConverterBase <WChar>
{
	DECLARE_CORE_INTERFACE

	typedef CodeSetConverterBase <WChar> Base;
public:
	/// Marshal string.
	/// 
	/// Default implementation does not perform any conversion.
	/// 
	/// \param [in, out] s The string.
	/// \param move `true` for move semantic.
	/// \param out The output stream.
	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out);

	/// Unmarshal string.
	/// 
	/// Default implementation does not perform any conversion.
	/// 
	/// \param in The input stream.
	/// \param [out] s The string.
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s);

	/// Marshal wide characters.
	/// 
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out);

	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data);

	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, Request& out);

	virtual void unmarshal_char_seq (Request& in, IDL::Sequence <WChar>& s);

	static CodeSetConverterW& get_default (unsigned GIOP_minor, bool marshal_responce) NIRVANA_NOEXCEPT;
};

/// Wide character converter GIOP 1.2 and later.
class NIRVANA_NOVTABLE CodeSetConverterW_1_2 :
	public CodeSetConverterW
{
public:
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out);
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data);
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out);
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s);
	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out);
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s);
};

/// Wide character converter GIOP 1.0. Rarely used. Only for compatibility.
class NIRVANA_NOVTABLE CodeSetConverterW_1_0 :
	public CodeSetConverterW
{
public:
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out);
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data);
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out);
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s);
	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out);
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s);

protected:
	CodeSetConverterW_1_0 (bool client_side) :
		marshal_minor_ (client_side ? MAKE_OMG_MINOR (5) : MAKE_OMG_MINOR (6))
	{}

private:
	ULong marshal_minor_;
};

}
}

#endif
