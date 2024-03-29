/// \file
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
#ifndef NIRVANA_ORB_CORE_PORTABLESERVER_CURRENT_H_
#define NIRVANA_ORB_CORE_PORTABLESERVER_CURRENT_H_
#pragma once

#include "PortableServer_Context.h"
#include <CORBA/PortableServer_s.h>

namespace PortableServer {
namespace Core {

class Current : public CORBA::servant_traits <PortableServer::Current>::Servant <Current>
{
public:
	static POA::_ref_type get_POA ()
	{
		return context ().adapter;
	}

	static ObjectId get_object_id ()
	{
		return context ().object_id;
	}

	static CORBA::Object::_ref_type get_reference ()
	{
		return context ().reference;
	}

	static PortableServer::ServantBase::_ref_type get_servant ()
	{
		// Must be called from the servant synchronization context.
		// Otherwise exception OBJ_ADAPTER will be thrown.
		return CORBA::Core::object2servant (context ().servant);
	}

private:
	static const Context& context ()
	{
		const Context* ctx = (const Context*)Nirvana::Core::ExecDomain::current ().TLS_get (
			Nirvana::Core::CoreTLS::CORE_TLS_PORTABLE_SERVER);
		if (!ctx)
			throw NoContext ();
		return *ctx;
	}
};

}
}

#endif
