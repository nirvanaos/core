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
#include "Packages.h"
#include "Binder.h"
#include <Nirvana/posix_defs.h>

namespace Nirvana {
namespace Core {

const char Packages::database_url_ [] = "file:/var/lib/packages.db?mode=r";

const char* const Packages::sys_module_names_ [MODULE_INSTALLER] = {
	"DecCalc.olf",
	"SFloat.olf",
	"dbc.olf",
	"SQLite.olf",
	"Installer.olf"
};

Packages::Packages (CORBA::Object::_ptr_type comp) :
	Servant (comp),
	name_service_ (CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (
		CORBA::Core::Services::NameService)))
{
	NDBC::Driver::_ref_type driver = NDBC::Driver::_narrow (
		load_and_bind (MODULE_SQLITE, false, "SQLite/driver"));

	try {
		connect (driver);
		NDBC::Statement::_ref_type st = connection_->createStatement (NDBC::ResultSet::Type::TYPE_FORWARD_ONLY);
		NDBC::ResultSet::_ref_type rs = st->executeQuery ("PRAGMA user_version;");
		rs->next ();
		int32_t cur_version = rs->getInt (1);
		if (database_version_ == cur_version)
			return;
	} catch (...) {
	}

	// Load installer to fix db
	get_installer ();
	// Retry connection
	connect (driver);
}

void Packages::connect (NDBC::Driver::_ptr_type driver)
{
	connection_ = driver->connect (database_url_, IDL::String (), IDL::String ());
}

CORBA::Object::_ref_type Packages::load_and_bind (SysModuleId id, bool singleton, const char* name) const
{
	return Binder::load_and_bind (id,
		get_system_binary_path (PLATFORM, sys_module_names_ [(int)id - 1]),
		name_service_, true, name);
}

IDL::traits <AccessDirect>::ref_type Packages::open_binary (
	CosNaming::NamingContextExt::_ptr_type ns, CORBA::Internal::String_in path)
{
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
	return AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
}

IDL::String Packages::get_system_binary_path (unsigned platform, const char* module_name)
{
	IDL::String path = Port::SysDomain::get_platform_dir (platform);
	IDL::String translated;
	if (FileSystem::translate_path (path, translated))
		path = std::move (translated);

	path += '/';
	path += module_name;

	return path;
}

Installer::_ref_type Packages::get_installer () const
{
	return Installer::_narrow (load_and_bind (MODULE_INSTALLER, true, "Nirvana/the_installer"));
}

}
}
