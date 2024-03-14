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
#ifndef NIRVANA_ORB_CORE_TC_VALUEBOX_H_
#define NIRVANA_ORB_CORE_TC_VALUEBOX_H_
#pragma once

#include "TC_ValueBase.h"
#include "TC_Impl.h"
#include "ORB.h"

namespace CORBA {
namespace Core {

class TC_ValueBox :
	public TC_Impl <TC_ValueBox, TC_ValueBase>
{
	typedef TC_Impl <TC_ValueBox, TC_ValueBase> Impl;

public:
	using TC_RefBase::_s_n_size;
	using TC_RefBase::_s_n_align;
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_content_type;
	using Servant::_s_get_compact_typecode;

	TC_ValueBox (IDL::String&& id, IDL::String&& name) noexcept :
		Impl (TCKind::tk_value_box, std::move (id), std::move (name))
	{}

	TC_ValueBox (IDL::String&& id, IDL::String&& name, TC_Ref&& content_type) noexcept :
		Impl (TCKind::tk_value_box, std::move (id), std::move (name)),
		content_type_ (std::move (content_type))
	{}

	void set_content_type (TC_Ref&& content_type) noexcept
	{
		content_type_ = std::move (content_type);
	}

	Boolean equal (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equal (TCKind::tk_value_box, id_, name_, content_type_, other);
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equivalent (TCKind::tk_value_box, id_, content_type_, other);
	}

	TypeCode::_ref_type content_type () const noexcept
	{
		return content_type_;
	}

	TypeCode::_ref_type get_compact_typecode ()
	{
		if (name_.empty ())
			return Impl::get_compact_typecode ();
		else
			return Static_the_orb::create_value_box_tc (id_, IDL::String (), content_type_);
	}

protected:
	virtual bool mark () noexcept override;
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept override;

private:
	TC_Ref content_type_;
};

}
}
#endif
