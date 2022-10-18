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
#ifndef NIRVANA_ORB_CORE_TC_IMPL_H_
#define NIRVANA_ORB_CORE_TC_IMPL_H_
#pragma once

#include <CORBA/Server.h>

namespace CORBA {
namespace Core {

template <class S, class Base>
class TC_Impl :
	public Base,
	public servant_traits <TypeCode>::Servant <S>,
	public Internal::LifeCycleRefCnt <S>
{
public:
	typedef servant_traits <TypeCode>::Servant <S> Servant;

	using Base::_s_id;
	using Base::_s_name;
	using Base::_s_member_count;
	using Base::_s_member_name;
	using Base::_s_member_type;
	using Base::_s_member_label;
	using Base::_s_discriminator_type;
	using Base::_s_default_index;
	using Base::_s_length;
	using Base::_s_content_type;
	using Base::_s_fixed_digits;
	using Base::_s_fixed_scale;
	using Base::_s_member_visibility;
	using Base::_s_type_modifier;
	using Base::_s_concrete_base_type;

	TypeCode::_ref_type get_compact_typecode () NIRVANA_NOEXCEPT
	{
		// For now just return this type code.
		// TODO: Implement.
		return Servant::_get_ptr ();
	}

protected:
	template <class ... Args>
	TC_Impl (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}
};

}
}

#endif
