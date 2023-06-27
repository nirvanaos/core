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

void Dir::bind (Name& n, Object::_ptr_type obj, bool rebind)
{
	FileSystem::set_error_number (0);
	if (Nirvana::File::_narrow (obj)) {
		try {
			check_name (n);
			bind_file (n, FileSystem::adapter ()->reference_to_id (obj), rebind);
			return;
		} catch (const RuntimeError& err) {
			FileSystem::set_error_number (err.error_number ());
		}
	} else
		throw BAD_PARAM ();
	throw CannotProceed (_this (), std::move (n));
}

void Dir::bind_context (Name& n, NamingContext::_ptr_type nc, bool rebind)
{
	FileSystem::set_error_number (0);
	if (Nirvana::Dir::_narrow (nc)) {
		try {
			check_name (n);
			bind_dir (n, FileSystem::adapter ()->reference_to_id (nc), rebind);
			return;
		} catch (const RuntimeError& err) {
			FileSystem::set_error_number (err.error_number ());
		}
	} else
		throw BAD_PARAM ();
	throw CannotProceed (_this (), std::move (n));
}

}
}
