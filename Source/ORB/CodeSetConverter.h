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

#include <CORBA/String.h>
#include "StreamIn.h"
#include "StreamOut.h"
#include "../StaticallyAllocated.h"

namespace CORBA {
namespace Core {

/// Code set converter for the narrow characters if other side encoding is not UTF-8.
/// Note that we convert only strings.
/// Single characters and characters array/sequences always must be ASCII.
class NIRVANA_NOVTABLE CodeSetConverter
{
	DECLARE_CORE_INTERFACE

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

	static Nirvana::Core::Ref <CodeSetConverter> get_default () noexcept
	{
		return &default_;
	}

	inline static void initialize () noexcept;

	static Nirvana::Core::StaticallyAllocated <Nirvana::Core::ImplStatic <CodeSetConverter> > default_;
};

/// Wide character marshal/unmarshal.
class NIRVANA_NOVTABLE CodeSetConverterW
{
	DECLARE_CORE_INTERFACE

public:
	/// Marshal string.
	///
	/// Default implementation does not perform any conversion.
	///
	/// \param [in, out] s The string.
	/// \param move `true` for move semantic.
	/// \param out The output stream.
	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out) = 0;

	/// Unmarshal string.
	///
	/// Default implementation does not perform any conversion.
	///
	/// \param in The input stream.
	/// \param [out] s The string.
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s) = 0;

	/// Marshal wide characters.
	/// 
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out) = 0;
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data) = 0;
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out) = 0;
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s) = 0;

	static Nirvana::Core::Ref <CodeSetConverterW> get_default (unsigned GIOP_minor, bool client_side);
};

/// Default wide code set converter for GIOP 1.0.
class NIRVANA_NOVTABLE CodeSetConverterW_1_0 :
	public CodeSetConverterW
{
public:
	static Nirvana::Core::Ref <CodeSetConverterW> get_default (bool client_side);

protected:
	// If a client orb erroneously sends wchar or wstring data in a GIOP 1.0 message, the server shall
	// generate a MARSHAL standard system exception, with standard minor code 5.
	// 
	// If a server erroneously sends wchar data in a GIOP 1.0 response, the client ORB shall raise a
	// MARSHAL exception to the client application with standard minor code 6.
	CodeSetConverterW_1_0 (bool client_side) :
		marshal_minor_ (client_side ? MAKE_OMG_MINOR (5) : MAKE_OMG_MINOR (6))
	{}

	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out) override;
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s) override;
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out) override;
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data) override;
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out) override;
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s) override;

private:
	const ULong marshal_minor_;
};

// Default wide code set converter for GIOP 1.1.

class NIRVANA_NOVTABLE CodeSetConverterW_1_1 :
	public CodeSetConverterW
{
public:
	static Nirvana::Core::Ref <CodeSetConverterW> get_default () noexcept
	{
		return &default_;
	}

	static Nirvana::Core::StaticallyAllocated <Nirvana::Core::ImplStatic <CodeSetConverterW_1_1> > default_;

protected:
	CodeSetConverterW_1_1 ()
	{}

	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out) override;
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s) override;
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out) override;
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data) override;
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out) override;
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s) override;
};

/// Default wide code set converter for GIOP 1.2.
class NIRVANA_NOVTABLE CodeSetConverterW_1_2 :
	public CodeSetConverterW
{
public:
	static Nirvana::Core::Ref <CodeSetConverterW> get_default () noexcept
	{
		return &default_;
	}

	static Nirvana::Core::StaticallyAllocated <Nirvana::Core::ImplStatic <CodeSetConverterW_1_2> > default_;

protected:
	CodeSetConverterW_1_2 ()
	{}

	virtual void marshal_string (IDL::WString& s, bool move, StreamOut& out) override;
	virtual void unmarshal_string (StreamIn& in, IDL::WString& s) override;
	virtual void marshal_char (size_t count, const WChar* data, StreamOut& out) override;
	virtual void unmarshal_char (StreamIn& in, size_t count, WChar* data) override;
	virtual void marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out) override;
	virtual void unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s) override;

	template <class U>
	static void write (U c, StreamOut& out)
	{
		unsigned cnt;
		if ((uint32_t)c > 0x00FFFFFF)
			cnt = 4;
		else if ((uint32_t)c > 0x0000FFFF)
			cnt = 3;
		else if ((uint32_t)c > 0x000000FF)
			cnt = 2;
		else if (c)
			cnt = 1;
		else
			cnt = 0;

		Octet ocnt = (Octet)cnt;
		out.write_c (1, 1, &ocnt);
		if (Nirvana::endian::native != Nirvana::endian::little)
			Internal::Type <U>::byteswap (c);
		out.write_c (1, cnt, &c);
	}

	static uint32_t read (StreamIn& in)
	{
		Octet ocnt;
		in.read (1, 1, &ocnt);
		uint32_t u;
		in.read (1, ocnt, &u);
		if (Nirvana::endian::native != Nirvana::endian::little)
			Internal::Type <uint32_t>::byteswap (u);
		return u;
	}
};

inline
void CodeSetConverter::initialize () noexcept
{
	CodeSetConverter::default_.construct ();
	CodeSetConverterW_1_1::default_.construct ();
	CodeSetConverterW_1_2::default_.construct ();
}

}
}

#endif
