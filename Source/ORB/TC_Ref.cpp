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
#include "TC_Ref.h"

namespace CORBA {
namespace Core {

TC_Ref::TC_Ref (TypeCode::_ptr_type p, TC_ComplexBase* complex) NIRVANA_NOEXCEPT :
	TypeCode::_ptr_type (p),
	complex_ (complex)
{
	if (!complex)
		Internal::interface_duplicate (p_);
}

TC_Ref::TC_Ref (TypeCode::_ptr_type p) NIRVANA_NOEXCEPT :
	TypeCode::_ptr_type (p),
	complex_ (nullptr)
{
	Internal::interface_duplicate (p_);
}

TC_Ref::TC_Ref (const TC_Ref& src) NIRVANA_NOEXCEPT :
	TypeCode::_ptr_type (src),
	complex_ (src.complex_)
{
	if (!complex_)
		Internal::interface_duplicate (p_);
}

TC_Ref& TC_Ref::operator = (const TC_Ref& src) NIRVANA_NOEXCEPT
{
	if (!complex_)
		Internal::interface_release (p_);
	p_ = src.p_;
	complex_ = src.complex_;
	if (!complex_)
		Internal::interface_duplicate (p_);
	return *this;
}

TC_Ref& TC_Ref::operator = (TC_Ref&& src) NIRVANA_NOEXCEPT
{
	if (!complex_)
		Internal::interface_release (p_);
	p_ = src.p_;
	complex_ = src.complex_;
	src.p_ = nullptr;
	src.complex_ = nullptr;
	return *this;
}

TC_Ref& TC_Ref::operator = (TypeCode::_ptr_type p) NIRVANA_NOEXCEPT
{
	TypeCode::_ptr_type::operator = (p);
	Internal::interface_duplicate (&p);
	complex_ = nullptr;
	return *this;
}

TC_Ref& TC_Ref::operator = (TypeCode::_ref_type&& src) NIRVANA_NOEXCEPT
{
	if (!complex_)
		Internal::interface_release (p_);
	reinterpret_cast <TypeCode::_ref_type&> (*this) = std::move (src);
	complex_ = nullptr;
	return *this;
}

}
}
