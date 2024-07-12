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
public:
	using CORBA::Internal::CCM_ObjectFeatures <SysDomain, Nirvana::SysDomain>::provide_facet;

	SysDomain (CORBA::Object::_ref_type& comp);
	~SysDomain ();

	static Version version () noexcept
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

	void get_binding (const IDL::String& name, unsigned platform, Binding& binding) const
	{
		if (get_sys_binding (name, platform, binding))
			return;

		// TODO:: Temporary solution, for testing
		ModuleLoad& module_load = binding.module_load ();
		module_load.binary (open_binary (get_sys_binary_path (platform, "TestModule.olf")));
		module_load.module_id (PM::MAX_SYS_MODULE_ID + 1);
	}

	PM::Packages::_ref_type provide_packages () const noexcept
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

	static void startup (CORBA::Object::_ptr_type obj);

private:
	static SysDomain* get_implementation (const CORBA::Core::ServantProxyLocal* proxy) noexcept
	{
		return static_cast <SysDomain*> (SysObjectLink::get_implementation (proxy));
	}

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

	enum SysModuleId
	{
		MODULE_DECCALC = 1,
		MODULE_SFLOAT,
		MODULE_DBC,
		MODULE_SQLITE,
		MODULE_PACKAGES,
		MODULE_SHELL
	};

	static AccessDirect::_ref_type open_sys_binary (unsigned platform, SysModuleId module_id);

	static bool get_sys_binding (CORBA::Internal::String_in name, unsigned platform, Binding& binding)
	{
		static const char dbc [] = "IDL:NDBC/";
		static const char dec_calc [] = "Nirvana/dec_calc";
		static const char sqlite_driver [] = "SQLite/driver";
		static const char sfloat_4 [] = "CORBA/Internal/sfloat_4";
		static const char sfloat_8 [] = "CORBA/Internal/sfloat_8";
		static const char sfloat_16 [] = "CORBA/Internal/sfloat_16";
		static const char regmod [] = "Nirvana/regmod";

		SysModuleId module_id;

		if (name == dec_calc) {
			module_id = MODULE_DECCALC;
		} else if (
			(!std::is_same <float, CORBA::Float>::value && name == sfloat_4)
			||
			(!std::is_same <double, CORBA::Double>::value && name == sfloat_8)
			||
			(!std::is_same <long double, CORBA::LongDouble>::value && name == sfloat_16)
			) {
			module_id = MODULE_SFLOAT;
		} else if (name.size () >= sizeof (dbc) && std::equal (dbc, dbc + sizeof (dbc) - 1, name.data ())) {
			module_id = MODULE_DBC;
		} else if (name == sqlite_driver) {
			module_id = MODULE_SQLITE;
		} else if (name == regmod) {
			module_id = MODULE_SHELL;
		} else
			return false;

		ModuleLoad& module_load = binding.module_load ();
		module_load.binary (open_sys_binary (platform, module_id));
		module_load.module_id (module_id);
		return true;
	}

	void do_startup (CORBA::Object::_ptr_type obj);

private:
	Nirvana::SysManager::_ref_type manager_;
	PM::Packages::_ref_type packages_;

	static const char* const sys_module_names_ [];
};

}
}

#endif
