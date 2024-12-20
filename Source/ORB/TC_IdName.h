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
#ifndef NIRVANA_ORB_CORE_TC_IDNAME_H_
#define NIRVANA_ORB_CORE_TC_IDNAME_H_
#pragma once

#include "TC_Base.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE TC_IdName : public TC_Base
{
public:
	TC_IdName (TCKind kind, IDL::String&& id, IDL::String&& name) noexcept;

	Boolean equal (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equal (kind_, id_, name_, other);
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equivalent (kind_, id_, other);
	}

	EqResult equivalent_ (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equivalent_ (kind_, id_, other);
	}

	IDL::String id () const
	{
		return id_;
	}

	IDL::String name () const
	{
		return name_;
	}

protected:
	const IDL::String id_;
	const IDL::String name_;
};

}
}

#endif
