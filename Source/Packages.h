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
#include <Port/SystemInfo.h>
#include "NameService/FileSystem.h"
#include <Nirvana/ModuleMetadata.h>

namespace Nirvana {
namespace Core {

/// Package manager
class Packages :
	public CORBA::servant_traits <Nirvana::Packages>::Servant <Packages>
{
	typedef CORBA::servant_traits <Nirvana::Packages>::Servant <Packages> Servant;

	enum SysModuleId
	{
		MODULE_DECCALC = 1,
		MODULE_SFLOAT = 2,
		MODULE_DBC = 3,
		MODULE_SQLITE = 4
	};

public:
	Packages (CORBA::Object::_ptr_type comp);

	static int32_t get_sys_bind_info (CORBA::Internal::String_in obj_name, 
		unsigned major, unsigned minor, IDL::String& module_path)
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

		if (module_id)
			module_path = get_system_binary_path (PLATFORM, sys_module_names_ [module_id - 1]);

		return module_id;
	}

	void get_bind_info (const IDL::String& obj_name, unsigned platform, 
		unsigned major, unsigned minor, BindInfo& bind_info) const
	{
		// TODO:: Temporary solution, for testing
		ModuleLoad& module_load = bind_info.module_load ();
		module_load.binary (open_system_binary (platform, "TestModule.olf"));
		module_load.module_id (100);
	}

	void register_module (const CosNaming::Name& path, unsigned flags)
	{
		CORBA::Object::_ref_type obj;
		name_service_->bind (path, obj);
		Nirvana::File::_ref_type file = Nirvana::File::_narrow (obj);
		ModuleMetadata metadata = get_module_metadata (AccessBuf::_downcast (file->open (O_RDONLY, 0)->_to_value ()));
		if (!metadata.check ()) {
			BindError::Error err;
			err.info ().message (std::move (metadata.error));
			throw err;
		}
		if (std::find (Port::SystemInfo::supported_platforms (),
			Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT,
			metadata.platform) == Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT)
		{
			BindError::Error err;
//			err.info ().
		}
	}

	static IDL::traits <AccessDirect>::ref_type open_binary (CosNaming::NamingContextExt::_ptr_type ns,
		CORBA::Internal::String_in path);

private:
	static IDL::String get_system_binary_path (unsigned platform, const char* module_name);

	IDL::traits <AccessDirect>::ref_type open_system_binary (unsigned platform,
		const char* module_name) const
	{
		return open_binary (name_service_, get_system_binary_path (platform, module_name));
	}

	CORBA::Object::_ref_type load_and_bind (SysModuleId id, bool singleton, const char* name) const;

	static void create_database (NDBC::Statement::_ptr_type st);

private:
	CosNaming::NamingContextExt::_ref_type name_service_;
	NDBC::Connection::_ref_type connection_;

	static const char database_url_ [];
	static const char* const db_script_ [];

	static const char* const sys_module_names_ [];
};

}
}

#endif
