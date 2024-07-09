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
#include <Nirvana/CoreDomains_s.h>
#include "ORB/SysServant.h"
#include "ORB/system_services.h"
#include "ORB/Services.h"
#include <Port/SystemInfo.h>
#include <Port/SysDomain.h>
#include <Nirvana/File.h>
#include "NameService/FileSystem.h"

namespace Nirvana {
namespace Core {

/// System domain
class SysDomain :
	public CORBA::Core::SysServantImpl <SysDomain, Nirvana::SysDomainCore, Nirvana::SysDomain,
		Components::Navigation, Components::Receptacles, Components::Events, Components::CCMObject>
{
	enum SysModuleId
	{
		MODULE_DECCALC = 1,
		MODULE_SFLOAT = 2,
		MODULE_DBC = 3,
		MODULE_SQLITE = 4
	};

public:
	using CORBA::Internal::CCM_ObjectFeatures <SysDomain, Nirvana::SysDomain>::provide_facet;

	SysDomain (CORBA::Object::_ref_type& comp);
	~SysDomain ();

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

	Nirvana::SysDomain::_ref_type login (const IDL::String& user, const IDL::String& password)
	{
		return _this ();
	}

	static Nirvana::ProtDomain::_ref_type prot_domain ()
	{
		return Nirvana::ProtDomain::_narrow (
			CORBA::Core::Services::bind (CORBA::Core::Services::ProtDomain));
	}

	void get_bind_info (const IDL::String& obj_name, unsigned platform,
		unsigned major, unsigned minor, BindInfo& bind_info) const
	{
		if (get_sys_bind_info (obj_name, platform, major, minor, bind_info))
			return;

		// TODO:: Temporary solution, for testing
		ModuleLoad& module_load = bind_info.module_load ();
		module_load.binary (open_binary (get_sys_binary_path (platform, "TestModule.olf")));
		module_load.module_id (100);
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
	static AccessDirect::_ref_type open_binary (const IDL::String& module_path);

	static const IDL::String get_sys_binary_path (unsigned platform, const char* module_name)
	{
		IDL::String path = Port::SysDomain::get_platform_dir (platform);
		{
			IDL::String translated;
			if (FileSystem::translate_path (path, translated))
				path = std::move (translated);
		}
		path += '/';
		path += module_name;
		return path;
	}

	static bool get_sys_bind_info (CORBA::Internal::String_in obj_name, unsigned platform,
		unsigned major, unsigned minor, BindInfo& bind_info)
	{
		static const char dbc [] = "IDL:NDBC/";
		static const char dec_calc [] = "Nirvana/dec_calc";
		static const char sqlite_driver [] = "SQLite/driver";
		static const char sfloat_4 [] = "CORBA/Internal/sfloat_4";
		static const char sfloat_8 [] = "CORBA/Internal/sfloat_8";
		static const char sfloat_16 [] = "CORBA/Internal/sfloat_16";

		int32_t module_id = 0;

		if (obj_name == dec_calc) {
			module_id = MODULE_DECCALC;
		} else if (
			(!std::is_same <float, CORBA::Float>::value && obj_name == sfloat_4)
			||
			(!std::is_same <double, CORBA::Double>::value && obj_name == sfloat_8)
			||
			(!std::is_same <long double, CORBA::LongDouble>::value && obj_name == sfloat_16)
			) {
			module_id = MODULE_SFLOAT;
		} else if (obj_name.size () >= sizeof (dbc) && std::equal (dbc, dbc + sizeof (dbc) - 1, obj_name.data ())) {
			module_id = MODULE_DBC;
		} else if (obj_name == sqlite_driver) {
			module_id = MODULE_SQLITE;
		}

		if (module_id) {
			ModuleLoad& module_load = bind_info.module_load ();
			module_load.binary (open_binary (get_sys_binary_path (PLATFORM, sys_module_names_ [module_id - 1])));
			module_load.module_id (module_id);
			return true;
		}

		return false;
	}

private:
	Nirvana::SysManager::_ref_type manager_;
	Nirvana::Packages::_ref_type packages_;

	static const char* const sys_module_names_ [];
};

}
}

#endif
