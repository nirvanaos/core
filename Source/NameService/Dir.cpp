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
#include "Dir.h"

using namespace CosNaming;
using namespace CORBA;

namespace Nirvana {
namespace Core {

void Dir::check_exist ()
{
	if (_non_existent ())
		throw OBJECT_NOT_EXIST (make_minor_errno (ENOTDIR));
}

void Dir::check_name (const CosNaming::Name& n)
{
	check_exist ();
	Base::check_name (n);
}

inline
void Dir::bind_file (Name& n, Object::_ptr_type obj, bool rebind)
{
	DirItemId id = FileSystem::adapter ()->reference_to_id (obj);
	try {
		Base::bind_file (n, id, rebind);
	} catch (const OBJECT_NOT_EXIST&) {
		etherealize ();
		throw;
	}
}

void Dir::bind_dir (Name& n, Object::_ptr_type obj, bool rebind)
{
	DirItemId id = FileSystem::adapter ()->reference_to_id (obj);
	try {
		Base::bind_dir (n, id, rebind);
	} catch (const OBJECT_NOT_EXIST&) {
		etherealize ();
		throw;
	}
}

void Dir::bind (Name& n, Object::_ptr_type obj, bool rebind)
{
	check_name (n);
	if (Nirvana::File::_narrow (obj))
		bind_file (n, obj, rebind);
	else if (Nirvana::Dir::_narrow (obj))
		bind_dir (n, obj, rebind);
	else
		throw BAD_PARAM (make_minor_errno (ENOENT));
}

void Dir::bind_context (Name& n, NamingContext::_ptr_type nc, bool rebind)
{
	check_name (n);
	if (Nirvana::Dir::_narrow (nc))
		bind_dir (n, nc, rebind);
	else
		throw BAD_PARAM (make_minor_errno (ENOTDIR));
}

}
}
