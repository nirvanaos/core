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
#include "pch.h"
#include "Packages.h"
#include <Nirvana/posix_defs.h>

namespace Nirvana {
namespace Core {

IDL::traits <AccessDirect>::ref_type Packages::open_binary (
	CosNaming::NamingContextExt::_ptr_type ns, CORBA::Internal::String_in path)
{
	CORBA::Object::_ref_type obj;
	try {
		obj = ns->resolve_str (path);
	} catch (const CORBA::Exception& ex) {
		const CORBA::SystemException* se = CORBA::SystemException::_downcast (&ex);
		if (se)
			se->_raise ();
		else
			throw_OBJECT_NOT_EXIST ();
	}
	File::_ref_type file = File::_narrow (obj);
	if (!file)
		throw_UNKNOWN (make_minor_errno (EISDIR));
	return AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
}

}
}
