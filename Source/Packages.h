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
#include <Port/SysDomain.h>
#include "NameService/FileSystem.h"
#include "Binder.h"

namespace Nirvana {
namespace Core {

/// Package manager
class Packages :
	public CORBA::servant_traits <Nirvana::Packages>::Servant <Packages>,
	private Port::SysDomain
{
	typedef CORBA::servant_traits <Nirvana::Packages>::Servant <Packages> Servant;

public:
	Packages (CORBA::Object::_ptr_type comp) :
		Servant (comp),
		name_service_ (CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (
			CORBA::Core::Services::NameService)))
	{}

	void get_bind_info (const IDL::String& obj_name, unsigned platform, BindInfo& bind_info) const
	{
		int32_t module_id;
		const char* module_name;

		const char dbc [] = "IDL:NDBC/";

		if (obj_name == "Nirvana/g_dec_calc") {
			module_name = "DecCalc.olf";
			module_id = 1;
		} else if (
			(!std::is_same <float, CORBA::Float>::value && obj_name == "CORBA/Internal/g_sfloat4")
			||
			(!std::is_same <double, CORBA::Double>::value && obj_name == "CORBA/Internal/g_sfloat8")
			||
			(!std::is_same <long double, CORBA::LongDouble>::value && obj_name == "CORBA/Internal/g_sfloat16")
		) {
			module_name = "SFloat.olf";
			module_id = 2;
		} else if (obj_name.size () >= sizeof (dbc) && 0 == obj_name.compare (0, sizeof (dbc) - 1, dbc)) {
			module_name = "dbc.olf";
			module_id = 3;
		} else if (obj_name == "SQLite/driver") {
			module_name = "SQLite.olf";
			module_id = 4;
		} else {
			// TODO:: Temporary solution, for testing
			module_name = "TestModule.olf";
			module_id = 100;
		}

		ModuleLoad& module_load = bind_info.module_load ();
		module_load.binary (open_system_binary (platform, module_name));
		module_load.module_id (module_id);
	}

	static IDL::traits <AccessDirect>::ref_type open_binary (CosNaming::NamingContextExt::_ptr_type ns,
		CORBA::Internal::String_in path);

private:
	IDL::traits <AccessDirect>::ref_type open_system_binary (unsigned platform,
		const char* module_name) const
	{
		IDL::String path = Port::SysDomain::get_platform_dir (platform);
		IDL::String translated;
		if (FileSystem::translate_path (path, translated))
			path = std::move (translated);

		path += '/';
		path += module_name;

		return open_binary (name_service_, path);
	}

private:
	CosNaming::NamingContextExt::_ref_type name_service_;

};

}
}

#endif
