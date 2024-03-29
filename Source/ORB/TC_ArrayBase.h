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
#ifndef NIRVANA_ORB_CORE_TC_ARRAYBASE_H_
#define NIRVANA_ORB_CORE_TC_ARRAYBASE_H_
#pragma once

#include "TC_Base.h"
#include "TC_Ref.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE TC_ArrayBase : public TC_Complex <TC_Base>
{
public:
	Boolean equal (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equal (kind_, bound_, content_type_, other);
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equivalent (kind_, bound_, content_type_, other);
	}

	ULong length () const noexcept
	{
		return bound_;
	}

	TypeCode::_ref_type content_type () const noexcept
	{
		return content_type_;
	}

protected:
	TC_ArrayBase (TCKind kind) :
		TC_Complex <TC_Base> (kind)
	{}

	bool set_content_type (TC_Ref&& content_type, ULong bound);

	virtual bool mark () noexcept override;

private:
	static TCKind get_array_kind (TypeCode::_ptr_type tc);

protected:
	TC_Ref content_type_;
	ULong bound_;
};

}
}

#endif
