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

#include "POA_SystemId.h"

using namespace CORBA;
using namespace Nirvana;

namespace PortableServer {
namespace Core {

ObjectId POA_SystemId::unique_id (ServantBase* proxy)
{
	int timestamp = proxy->timestamp ();
	ObjectId oid;
	oid.resize (sizeof (proxy) + sizeof (timestamp));
	real_copy ((const Octet*)&timestamp, (const Octet*)(&timestamp + 1),
		real_copy ((const Octet*)&proxy, (const Octet*)(&proxy + 1), oid.data ()));
	return oid;
}

}
}

