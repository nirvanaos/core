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
#include "POA.h"

using namespace std;
using namespace CORBA;

namespace PortableServer {
namespace Core {

AOM::const_iterator POA_Root::AOM_insert (Object::_ptr_type proxy, ObjectId& objid)
{
	assert (proxy);
	auto servant = object2servant_core (proxy);
	int timestamp = servant->timestamp ();
	objid.resize (sizeof (&servant) + sizeof (timestamp));
	Nirvana::real_copy ((const Octet*)&timestamp, (const Octet*)(&timestamp + 1),
		Nirvana::real_copy ((const Octet*)&servant, (const Octet*)(&servant + 1), objid.data ()));
	auto ins = active_object_map_.emplace (objid, proxy);
	assert (ins.second);
	if (!ins.second)
		throw CORBA::OBJ_ADAPTER ();
	return ins.first;
}

}
}
