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
#ifndef NIRVANA_ORB_CORE_TC_VALUE_H_
#define NIRVANA_ORB_CORE_TC_VALUE_H_
#pragma once

#include "TC_ValueBase.h"
#include "TC_Impl.h"
#include "../Array.h"
#include "../UserAllocator.h"

namespace CORBA {
namespace Core {

class TC_Value :
	public TC_Impl <TC_Value, TC_ValueBase>
{
	typedef TC_Impl <TC_Value, TC_ValueBase> Impl;

public:
	using TC_RefBase::_s_n_size;
	using TC_RefBase::_s_n_align;
	using TC_RefBase::_s_n_is_CDR;
	using TC_RefBase::_s_n_byteswap;
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_member_count;
	using Servant::_s_member_name;
	using Servant::_s_member_type;
	using Servant::_s_member_visibility;
	using Servant::_s_type_modifier;
	using Servant::_s_concrete_base_type;

	struct Member
	{
		IDL::String name;
		TC_Ref type;
		Short visibility;
	};

	typedef Nirvana::Core::Array <Member, Nirvana::Core::UserAllocator> Members;

	TC_Value (IDL::String&& id, IDL::String&& name, ValueModifier modifier) NIRVANA_NOEXCEPT :
		Impl (TCKind::tk_value, std::move (id), std::move (name)),
		modifier_ (modifier)
	{}

	TC_Value (IDL::String&& id, IDL::String&& name, ValueModifier modifier, TC_Ref&& concrete_base,
		Members&& members) NIRVANA_NOEXCEPT :
		Impl (TCKind::tk_value, std::move (id), std::move (name)),
		modifier_ (modifier),
		concrete_base_ (std::move (concrete_base)),
		members_ (std::move (members))
	{
		TC_Ref self (_get_ptr (), this);
		for (auto& m : members_) {
			m.type.replace_recursive_placeholder (id_, self);
		}
	}

	void set_members (TC_Ref&& concrete_base, Members&& members) NIRVANA_NOEXCEPT
	{
		concrete_base_ = std::move (concrete_base);
		members_ = std::move (members);
	}

	Boolean equal (TypeCode::_ptr_type other)
	{
		if (!TC_IdName::equal (other))
			return false;

		if (!equivalent_members (other))
			return false;

		for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
			if (other->member_name (i) != members_ [i].name)
				return false;
		}
		return true;
	}

	Boolean equivalent (TypeCode::_ptr_type other)
	{
		TypeCode::_ptr_type tc = dereference_alias (other);
		EqResult eq = TypeCodeBase::equivalent_ (kind_, id_, tc);
		if (EqResult::UNKNOWN != eq)
			return eq == EqResult::YES;

		return equivalent_members (tc);
	}

	ValueModifier type_modifier () const NIRVANA_NOEXCEPT
	{
		return modifier_;
	}

	TypeCode::_ref_type concrete_base_type () const NIRVANA_NOEXCEPT
	{
		return concrete_base_;
	}

	ULong member_count () const NIRVANA_NOEXCEPT
	{
		return (ULong)members_.size ();
	}

	IDL::String member_name (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i].name;
	}

	TypeCode::_ref_type member_type (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i].type;
	}

	Short member_visibility (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i].visibility;
	}

protected:
	virtual bool mark () NIRVANA_NOEXCEPT override;
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT override;

private:
	bool equivalent_members (TypeCode::_ptr_type other);

private:
	ValueModifier modifier_;
	TC_Ref concrete_base_;
	Members members_;
};

}
}

#endif