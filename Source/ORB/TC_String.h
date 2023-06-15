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
#ifndef NIRVANA_ORB_CORE_TC_STRING_H_
#define NIRVANA_ORB_CORE_TC_STRING_H_
#pragma once

#include "TC_Base.h"
#include "TC_Impl.h"

namespace CORBA {
namespace Core {

class TC_StringBase : public TC_Base
{
public:
	Boolean equal (TypeCode::_ptr_type other) const
	{
		return kind_ == other->kind () && bound_ == other->length ();
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return equal (dereference_alias (other));
	}

	ULong length () const noexcept
	{
		return bound_;
	}

protected:
	TC_StringBase (TCKind kind, ULong bound) noexcept :
		TC_Base (kind),
		bound_ (bound)
	{}

protected:
	const ULong bound_;
};

template <typename C>
class TC_StringT :
	public TC_Impl <TC_StringT <C>, TC_StringBase>,
	public Internal::TypeCodeOps <Internal::StringT <C> >
{
	typedef TC_Impl <TC_StringT <C>, TC_StringBase> Impl;
	typedef Internal::TypeCodeOps <Internal::StringT <C> > Ops;

public:
	using Impl::Servant::_s_length;
	using Ops::_s_n_aligned_size;
	using Ops::_s_n_CDR_size;
	using Ops::_s_n_align;
	using Ops::_s_n_is_CDR;
	using TC_Base::_s_n_byteswap;

	TC_StringT (ULong bound) noexcept;
};

template <> inline
TC_StringT <Char>::TC_StringT (ULong bound) noexcept :
	Impl (TCKind::tk_string, bound)
{}

template <> inline
	TC_StringT <WChar>::TC_StringT (ULong bound) noexcept :
	Impl (TCKind::tk_wstring, bound)
{}

typedef TC_StringT <Char> TC_String;
typedef TC_StringT <WChar> TC_WString;

}
}

#endif
