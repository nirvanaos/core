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

namespace CORBA {
namespace Core {

class TC_ValueBox :
	public TC_Impl <TC_ValueBox, TC_ValueBase>
{
	typedef TC_Impl <TC_ValueBox, TC_ValueBase> Impl;

public:
	using TC_RefBase::_s_n_size;
	using TC_RefBase::_s_n_align;
	using TC_RefBase::_s_n_is_CDR;
	using TC_RefBase::_s_n_byteswap;
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_content_type;

	TC_ValueBox (IDL::String&& id, IDL::String&& name) NIRVANA_NOEXCEPT :
		Impl (TCKind::tk_value_box, std::move (id), std::move (name))
	{}

	TC_ValueBox (IDL::String&& id, IDL::String&& name, TC_Ref&& content_type) NIRVANA_NOEXCEPT :
		Impl (TCKind::tk_value_box, std::move (id), std::move (name)),
		content_type_ (std::move (content_type))
	{}

	void set_content_type (TC_Ref&& content_type) NIRVANA_NOEXCEPT
	{
		content_type_ = std::move (content_type_);
	}

	TypeCode::_ref_type content_type () const NIRVANA_NOEXCEPT
	{
		return content_type_;
	}

protected:
	virtual bool mark () NIRVANA_NOEXCEPT override;
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT override;

private:
	TC_Ref content_type_;
};

}
}
#endif
