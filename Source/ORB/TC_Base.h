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
#ifndef NIRVANA_ORB_CORE_TC_BASE_H_
#define NIRVANA_ORB_CORE_TC_BASE_H_
#pragma once

#include <CORBA/Proxy/TypeCodeImpl.h>
#include "LifeCyclePseudo.h"

namespace CORBA {
namespace Core {

template <class S>
class TC_Base :
	public servant_traits <TypeCode>::Servant <S>,
	public Nirvana::Core::LifeCyclePseudo <S>,
	public CORBA::Internal::TypeCodeBase
{
public:
	typedef servant_traits <TypeCode>::Servant <S> Servant;
	typedef CORBA::Internal::TypeCodeBase BaseImpl;
	using BaseImpl::_s_id;
	using BaseImpl::_s_name;
	using BaseImpl::_s_member_count;
	using BaseImpl::_s_member_name;
	using BaseImpl::_s_member_type;
	using BaseImpl::_s_member_label;
	using BaseImpl::_s_discriminator_type;
	using BaseImpl::_s_default_index;
	using BaseImpl::_s_length;
	using BaseImpl::_s_content_type;
	using BaseImpl::_s_fixed_digits;
	using BaseImpl::_s_fixed_scale;
	using BaseImpl::_s_member_visibility;
	using BaseImpl::_s_type_modifier;
	using BaseImpl::_s_concrete_base_type;

	TCKind kind () const NIRVANA_NOEXCEPT
	{
		return kind_;
	}
	
	TypeCode::_ref_type get_compact_typecode () NIRVANA_NOEXCEPT
	{
		// By default just return this type code.
		return Servant::_get_ptr ();
	}

protected:
	TC_Base (TCKind kind) :
		kind_ (kind)
	{}

protected:
	const TCKind kind_;
};

}
}

#endif
