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
#ifndef NIRVANA_ORB_CORE_TC_REF_H_
#define NIRVANA_ORB_CORE_TC_REF_H_
#pragma once

#include <CORBA/CORBA.h>

namespace CORBA {
namespace Core {

class TC_Ref : public TypeCode::_ptr_type
{
public:
	TC_Ref () :
		strong_ (false)
	{}

	TC_Ref (TypeCode::_ptr_type p, bool strong) NIRVANA_NOEXCEPT :
		TypeCode::_ptr_type (p),
		strong_ (strong)
	{
		if (strong)
			Internal::interface_duplicate (&p);
	}

	TC_Ref (TC_Ref&& src) NIRVANA_NOEXCEPT :
		TypeCode::_ptr_type (src),
		strong_ (src.strong_)
	{
		src.strong_ = false;
	}

	~TC_Ref ()
	{
		if (strong_)
			Internal::interface_release (p_);
	}

	TC_Ref& operator = (TC_Ref&& src) NIRVANA_NOEXCEPT
	{
		if (strong_)
			Internal::interface_release (p_);
		p_ = src.p_;
		strong_ = src.strong_;
		src.strong_ = false;
		return *this;
	}

	TC_Ref& operator = (TypeCode::_ptr_type p) NIRVANA_NOEXCEPT
	{
		TypeCode::_ptr_type::operator = (p);
		Internal::interface_duplicate (&p);
		strong_ = true;
		return *this;
	}

	TC_Ref& operator = (TypeCode::_ref_type&& src) NIRVANA_NOEXCEPT
	{
		reinterpret_cast <TypeCode::_ref_type&> (*this) = std::move (src);
		return *this;
	}

private:
	bool strong_;
};

}
}

#endif
