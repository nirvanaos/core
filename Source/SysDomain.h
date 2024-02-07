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
#include <Nirvana/Domains_s.h>
#include "ORB/SysServant.h"
#include "ORB/system_services.h"
#include "ORB/Services.h"
#include <Port/SystemInfo.h>

namespace Nirvana {
namespace Core {

/// System domain
class SysDomain :
	public CORBA::Core::SysServantImpl <SysDomain, Nirvana::SysDomain>
{
public:
	SysDomain ();

	~SysDomain ()
	{}

	static SysDomain& implementation ();

	static Version version ()
	{
		return { 0, 0, 0, 0 };
	}

	static Platforms supported_platforms ()
	{
		return Platforms (Port::SystemInfo::supported_platforms (),
			Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT);
	}

	Nirvana::SysDomain::_ref_type connect (const IDL::String& user, const IDL::String& password)
	{
		return _this ();
	}

	static Nirvana::ProtDomain::_ref_type prot_domain ()
	{
		return Nirvana::ProtDomain::_narrow (
			CORBA::Core::Services::bind (CORBA::Core::Services::ProtDomain));
	}

	Nirvana::Packages::_ref_type provide_packages () const noexcept
	{
		return packages_;
	}

	Nirvana::SysManager::_ref_type provide_manager () const noexcept
	{
		return manager_;
	}

	CORBA::Object::_ref_type get_service (const IDL::String& id)
	{
		return CORBA::Core::Services::bind (id);
	}

private:
	Nirvana::SysManager::_ref_type manager_;
	Nirvana::Packages::_ref_type packages_;
};

}
}

#endif
