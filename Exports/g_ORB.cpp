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
#include <CORBA/Server.h>
#include <ORB/IDL/ORB_s.h>
#include <Binder.h>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

/// Implementation of the CORBA::ORB interface.
class ORB :
	public CORBA::servant_traits <CORBA::ORB>::ServantStatic <ORB>
{
public:
	static Object::_ref_type resolve_initial_references (const ObjectId& identifier)
	{
		return Binder::bind_service (identifier);
	}

};

}

__declspec (selectany)
const ImportInterfaceT <ORB> g_ORB = { OLF_IMPORT_INTERFACE,
	"CORBA/g_ORB", CORBA::Internal::RepIdOf <ORB>::repository_id_,
	NIRVANA_STATIC_BRIDGE (ORB, Core::ORB) };

}

NIRVANA_EXPORT (_exp_CORBA_g_ORB, "CORBA/g_ORB", CORBA::ORB, CORBA::Core::ORB)
