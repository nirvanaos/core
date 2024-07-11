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
#include "open_binary.h"
#include <Nirvana/posix_defs.h>
#include "BindError.h"

namespace Nirvana {
namespace Core {

AccessDirect::_ref_type open_binary (CosNaming::NamingContextExt::_ptr_type ns, const IDL::String& path)
{
	CORBA::Object::_ref_type obj;
	try {
		obj = ns->resolve_str (path);
	} catch (const CosNaming::NamingContext::NotFound&) {
		BindError::throw_message ("Path not found: " + path);
	}
	File::_ref_type file = File::_narrow (obj);
	if (!file)
		throw_UNKNOWN (make_minor_errno (EISDIR));
	return AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
}

}
}
