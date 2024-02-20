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
#include <Port/SysDomain.h>
#include "NameService/FileSystem.h"
#include <fnctl.h>

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
		Servant (comp)
	{}

	void get_bind_info (const IDL::String& obj_name, unsigned platform, BindInfo& bind_info)
	{
		// TODO:: Temporary solution, for testing
		IDL::String path = Port::SysDomain::get_platform_dir (platform);
		IDL::String translated;
		if (FileSystem::translate_path (path, translated))
			path = std::move (translated);
		auto ns = CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::NameService));

		int32_t module_id;
		const char* module_name;
		if (obj_name == "Nirvana/g_dec_calc") {
			module_name = "DecCalc.olf";
			module_id = 1;
		} else if (
			(!std::is_same <float, CORBA::Float>::value && obj_name == "CORBA/Internal/g_sfloat4")
			||
			(!std::is_same <double, CORBA::Double>::value && obj_name == "CORBA/Internal/g_sfloat8")
			||
			(!std::is_same <long double, CORBA::LongDouble>::value && obj_name == "CORBA/Internal/g_sfloat16")
			)
		{
			module_name = "SFloat.olf";
			module_id = 2;
		} else {
			module_name = "TestModule.olf";
			module_id = 100;
		}

		path += '/';
		path += module_name;
		CORBA::Object::_ref_type obj;
		try {
			obj = ns->resolve_str (path);
		} catch (const CORBA::Exception& ex) {
			const CORBA::SystemException* se = CORBA::SystemException::_downcast (&ex);
			if (se)
				se->_raise ();
			else
				throw_OBJECT_NOT_EXIST ();
		}
		File::_ref_type file = File::_narrow (obj);
		if (!file)
			throw_UNKNOWN (make_minor_errno (EISDIR));
		ModuleLoad& module_load = bind_info.module_load ();
		module_load.binary (AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ()));
		module_load.module_id (module_id);
	}

};

}
}

#endif
