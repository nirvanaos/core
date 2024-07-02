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
#include "Nirvana/CoreDomains.h"

#define DATABASE_PATH "/var/lib/packages.db"
#define DATABASE_VERSION 1

#define STRINGIZE0(n) #n
#define STRINGIZE(n) STRINGIZE0 (n)

namespace Nirvana {
namespace Core {

const char Packages::database_url_ [] = "file:/var/lib/packages.db?mode=ro";

const char* const Packages::db_script_ [] = {
"CREATE TABLE module(name TEXT UNIQUE,id INTEGER PRIMARY KEY AUTOINCREMENT,flags INTEGER);"

"CREATE TABLE export(name TEXT NOT NULL,major INTEGER NOT NULL,minor INTEGER NOT NULL,flags INTEGER NOT NULL,"
"module INTEGER NOT NULL REFERENCES module(id)ON DELETE CASCADE,"
"PRIMARY KEY(name, major));"

"CREATE TABLE binary("
"module INTEGER NOT NULL REFERENCES module(id)ON DELETE CASCADE,"
"platform INTEGER NOT NULL,path TEXT NOT NULL,UNIQUE(module,platform));",

"INSERT INTO module VALUES('100', 100, 0);"
"DELETE FROM module WHERE id=100;"
"PRAGMA user_version=" STRINGIZE (DATABASE_VERSION) ";"
};

const char* const Packages::sys_module_names_ [MODULE_SQLITE] = {
	"DecCalc.olf",
	"SFloat.olf",
	"dbc.olf",
	"SQLite.olf"
};

Packages::Packages (CORBA::Object::_ptr_type comp) :
	Servant (comp),
	name_service_ (CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (
		CORBA::Core::Services::NameService)))
{
	NDBC::Driver::_ref_type driver = NDBC::Driver::_narrow (
		load_and_bind (MODULE_SQLITE, false, "SQLite/driver"));

	connection_ = driver->connect ("file:" DATABASE_PATH "?mode=rwc&journal_mode=WAL",
		IDL::String (), IDL::String ());

	NDBC::Statement::_ref_type st = connection_->createStatement (NDBC::ResultSet::Type::TYPE_FORWARD_ONLY);
	NDBC::ResultSet::_ref_type rs = st->executeQuery ("PRAGMA user_version;");
	rs->next ();
	int32_t cur_version = rs->getInt (1);
	if (DATABASE_VERSION != cur_version)
		create_database (st);
}

inline void Packages::create_database (NDBC::Statement::_ptr_type st)
{
	for (const char* sql : db_script_) {
		st->executeUpdate (sql);
	}
}

CORBA::Object::_ref_type Packages::load_and_bind (SysModuleId id, bool singleton, const char* name) const
{
	return Binder::load_and_bind (id,
		get_system_binary_path (PLATFORM, sys_module_names_ [(int)id - 1]),
		name_service_, singleton, name);
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

}
}
