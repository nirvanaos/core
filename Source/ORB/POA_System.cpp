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
#include "POA_Root.h"

namespace PortableServer {
namespace Core {

ObjectId POA_System::generate_object_id ()
{
	const CORBA::Octet* p = (const CORBA::Octet*)&next_id_;
	ObjectId ret (p, p + sizeof (ID));
	++next_id_;
	return ret;
}

void POA_System::check_object_id (const ObjectId& oid)
{
	if (oid.size () != sizeof (ID))
		throw CORBA::BAD_PARAM ();
	ID id = *(const ID*)oid.data ();
	if (id >= next_id_)
		throw CORBA::BAD_PARAM ();
}

ObjectId POA_SystemPersistent::generate_object_id ()
{
	return root_->generate_persistent_id ();
}

void POA_SystemPersistent::check_object_id (const ObjectId& oid)
{
	POA_Root::check_persistent_id (oid);
}

}
}
