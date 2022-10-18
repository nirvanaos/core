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
#include "TC_Alias.h"

namespace CORBA {
namespace Core {

TC_Alias::TC_Alias (IDL::String&& id, IDL::String&& name, TC_Ref&& content_type) NIRVANA_NOEXCEPT :
	Impl (TCKind::tk_alias, std::move (id), std::move (name)),
	content_type_ (std::move (content_type))
{}

bool TC_Alias::mark () NIRVANA_NOEXCEPT
{
	if (!TC_ComplexBase::mark ())
		return false;
	content_type_.mark ();
	return true;
}

}
}
