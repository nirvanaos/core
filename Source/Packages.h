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
#ifndef NIRVANA_CORE_PACKAGES_H_
#define NIRVANA_CORE_PACKAGES_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/Domains_s.h>
#include <Nirvana/NDBC.h>
#include <Nirvana/posix_defs.h>
#include <Port/SystemInfo.h>
#include <Port/Module.h>
#include "Nirvana/CoreDomains.h"
#include "BindError.h"

namespace Nirvana {
namespace Core {

/// Package manager
class Packages :
	public CORBA::servant_traits <Nirvana::Packages>::Servant <Packages>
{
	typedef CORBA::servant_traits <Nirvana::Packages>::Servant <Packages> Servant;

public:
	Packages (CORBA::Object::_ptr_type comp);

	IDL::String get_module_name (int32_t id) const
	{
		//		NDBC::PreparedStatement::_ref_type st =
		//			connection_->prepareStatement ("SELECT name FROM module WHERE id=?", NDBC::ResultSet::Type::TYPE_FORWARD_ONLY, 0);
		//		st->setInt (1, id);
		//		NDBC::ResultSet::_ref_type rs = st->executeQuery ();

		IDL::String name;
		//		if (rs->next ())
		//			name = rs->getString (1);

		return name;
	}

	uint16_t register_module (const CosNaming::Name& path, const IDL::String& name, unsigned flags)
	{
		AccessDirect::_ref_type binary;

		{
			CORBA::Object::_ref_type obj = name_service_->resolve (path);
			Nirvana::File::_ref_type file = Nirvana::File::_narrow (obj);
			binary = AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
		}

		uint16_t platform = Port::Module::get_platform (binary);
		if (std::find (Port::SystemInfo::supported_platforms (),
			Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT,
			platform) == Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT)
		{
			BindError::throw_message ("Unsupported platform");
		}

		Nirvana::SysDomain::_ref_type sys_domain = Nirvana::SysDomain::_narrow (
			CORBA::the_orb->resolve_initial_references ("SysDomain"));

		Nirvana::ProtDomain::_ref_type bind_domain;
		if (SINGLE_DOMAIN)
			bind_domain = sys_domain->prot_domain ();
		else
			bind_domain = sys_domain->provide_manager ()->create_prot_domain (platform);
		
		ModuleBindings module_bindings =
			ProtDomainCore::_narrow (bind_domain)->get_module_bindings (binary);

		if (!SINGLE_DOMAIN)
			bind_domain->shutdown (0);

		return platform;
	}

private:
	static void create_database (NDBC::Statement::_ptr_type st);

	struct SemVer
	{
		uint64_t version;
		std::string name;
		std::string prerelease;

		SemVer (const std::string& mod_name);
	};

private:
	CosNaming::NamingContextExt::_ref_type name_service_;
	NDBC::Connection::_ref_type connection_;

	static const char database_url_ [];
	static const char* const db_script_ [];
};

}
}

#endif
