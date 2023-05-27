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
#include "ORB/SysServant.h"
#include "ORB/Services.h"
#include <Nirvana/CoreDomains_s.h>
#include <Port/SystemInfo.h>

namespace Nirvana {
namespace Core {

/// System domain
class SysDomain :
	public CORBA::Core::SysServantImpl <SysDomain, 0, SysDomainCore, Nirvana::SysDomain>
{
public:
	~SysDomain ()
	{}

	static Version version ()
	{
		return { 0, 0, 0, 0 };
	}

	static Platforms supported_platforms ()
	{
		return Platforms (std::begin (Port::SystemInfo::supported_platforms_),
			std::end (Port::SystemInfo::supported_platforms_));
	}

	static Nirvana::ProtDomain::_ref_type prot_domain ()
	{
		return Nirvana::ProtDomain::_narrow (
			CORBA::Core::Services::bind (CORBA::Core::Services::ProtDomain));
	}

	static ProtDomain::_ref_type create_prot_domain (uint16_t platform)
	{
		throw_NO_IMPLEMENT ();
	}

	static ProtDomain::_ref_type create_prot_domain_as_user (uint16_t platform,
		const IDL::String& user, const IDL::String& password)
	{
		throw_NO_IMPLEMENT ();
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
