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
#include <CORBA/Server.h>
#include <Nirvana/Module_s.h>

namespace Nirvana {
namespace Core {

class Module :
	public ::CORBA::Nirvana::ServantStatic <Module, ::Nirvana::Module>
{
public:
	static const void* base_address ()
	{
		return nullptr;
	}

	template <class I>
	static CORBA::Nirvana::Interface* __duplicate (CORBA::Nirvana::Interface* itf, CORBA::Nirvana::Interface*)
	{
		return itf;
	}

	template <class I>
	static void __release (CORBA::Nirvana::Interface*)
	{}
};

}

extern const ImportInterfaceT <Module> g_module = { OLF_IMPORT_INTERFACE, "Nirvana/g_module", Module::repository_id_, NIRVANA_STATIC_BRIDGE (Module, Core::Module) };

}
