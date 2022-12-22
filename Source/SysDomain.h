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
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_
#pragma once

#include <CORBA/Server.h>
#include "Nirvana/Domains_s.h"
#include "ORB/Services.h"

namespace Nirvana {
namespace Core {

/// System domain.
class SysDomain :
	public CORBA::servant_traits <Nirvana::SysDomain>::Servant <SysDomain>
{
public:
	~SysDomain ()
	{}

	static PortableServer::POA::_ref_type _default_POA () NIRVANA_NOEXCEPT
	{
		// Disable implicit activation
		return PortableServer::POA::_nil ();
	}

	void get_bind_info (const IDL::String& obj_name, unsigned platform, BindInfo& bind_info)
	{
		if (obj_name == "Nirvana/g_dec_calc")
			bind_info.module_name ("DecCalc.olf");
		else
			bind_info.module_name ("TestModule.olf");
	}

	CORBA::Object::_ref_type get_service (const IDL::String& id)
	{
		return CORBA::Core::Services::bind (id);
	}
};

}
}

#endif
