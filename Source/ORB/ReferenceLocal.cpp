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
#include "ReferenceLocal.h"

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <Nirvana::Core::AtomicCounter <false> > ReferenceLocal::next_timestamp_;

void ReferenceLocal::marshal (Internal::String_in primary_iid, StreamOut& out) const
{
	out.write_string (static_cast <const IDL::String&> (primary_iid));
	throw NO_IMPLEMENT (); // TODO: Implement.
	PortableServer::Core::ObjectKeyRef ref = object_key_.get ();
	if (!ref)
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	ref->marshal (out);
}

}
}
