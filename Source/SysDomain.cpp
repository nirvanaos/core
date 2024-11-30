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
#include "pch.h"
#include "SysDomain.h"
#include "ORB/ORB.h"
#include "SysManager.h"
#include "NameService/FileSystem.h"

using namespace CORBA;

namespace Nirvana {
namespace Core {

const char* const SysDomain::sys_module_names_ [MODULE_SHELL] = {
	"DecCalc.olf",
	"SFloat.olf",
	"dbc.olf",
	"SQLite.olf",
	"PacMan.olf",
	"shell.olf"
};

inline
SysDomain::SysDomain ()
{
	// Create SysManager
	SYNC_BEGIN (g_core_free_sync_context, &MemContext::current ());
	manager_ = make_reference <SysManager> (_object ())->_this ();
	SYNC_END ();
}

SysDomain::~SysDomain ()
{}

Object::_ref_type create_SysDomain ()
{
	// Use make_stateless to create object in the free sync context
	// The IDL API does not change state of the SysDomain servant.
	// The only do_startup () method is change state but it is called only once in StartupSys::run ().
	if (ESIOP::is_system_domain ())
		return make_stateless <SysDomain> ()->_this ();
	else
		return CORBA::Static_the_orb::string_to_object (
			"corbaloc::1.1@/%00", CORBA::Internal::RepIdOf <SysDomain::PrimaryInterface>::id);
}

inline void SysDomain::do_startup (Object::_ptr_type obj)
{
	// Create Packages object
	// This method called once from StartupSys::run ().
	Binder::BindResult br = Binder::load_and_bind (MODULE_PACKAGES,
		open_sys_binary (PLATFORM, MODULE_PACKAGES),
		"Nirvana/PM/pac_factory", CORBA::Internal::RepIdOf <PM::PacFactory>::id);
	PM::PacFactory::_ref_type pf = std::move (br.itf).template downcast <PM::PacFactory> ();
	SYNC_BEGIN (*br.sync_context, &MemContext::current ());
	packages_ = pf->create (obj);
	SYNC_END ();
}

void SysDomain::startup (Object::_ptr_type obj)
{
	get_implementation (get_proxy (obj))->do_startup (obj);
}

AccessDirect::_ref_type SysDomain::open_sys_binary (unsigned platform, SysModuleId module_id)
{
	return open_binary (get_sys_binary_path (platform, sys_module_names_ [module_id - 1]));
}

}
}
