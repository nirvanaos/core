/*
* Nirvana Installer module.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2024 Igor Popov.
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
#include "../Source/Nirvana/CoreDomains_s.h"
#include <Nirvana/NDBC.h>

#define DATABASE_PATH "/var/lib/packages.db"
#define DATABASE_VERSION 1

#define STRINGIZE0(n) #n
#define STRINGIZE(n) STRINGIZE0 (n)

using namespace NDBC;

namespace Nirvana {

static const char db_script [] =
"BEGIN;"

"CREATE TABLE IF NOT EXISTS packages(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT UNIQUE);"

"CREATE TABLE IF NOT EXISTS modules(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT UNIQUE,"
"flags INTEGER);"

"CREATE TABLE IF NOT EXISTS mod2pack(package INTEGER REFERENCES packages(id),"
"module INTEGER REFERENCES modules(id));"

"CREATE TABLE IF NOT EXISTS binaries(module INTEGER REFERENCES modules(id),platform INTEGER,"
"path TEXT UNIQUE, UNIQUE(module,platform));"

"CREATE TABLE IF NOT EXISTS objects(name TEXT, version INTEGER,"
"module INTEGER REFERENCES modules(id), flags INTEGER, PRIMARY KEY(name,version));"

"PRAGMA user_version=" STRINGIZE (DATABASE_VERSION) ";"

"END;";

class InstallerImpl : public CORBA::servant_traits <Installer>::Servant <InstallerImpl>
{
public:
	static Installer::_ref_type get_singleton ()
	{
		CORBA::servant_reference <InstallerImpl> servant;
		if (!singleton_)
			servant = CORBA::make_reference <InstallerImpl> ();
		else
			servant = singleton_;
		return servant->_this ();
	}

	void register_module (const CosNaming::Name& path, unsigned flags)
	{}

	InstallerImpl ()
	{
		singleton_ = this;

		connection_ = SQLite::driver->connect ("file:" DATABASE_PATH "?mode=rwc&journal_mode=WAL", "", "");
		Statement::_ref_type st = connection_->createStatement (ResultSet::Type::TYPE_FORWARD_ONLY);

		ResultSet::_ref_type rs = st->executeQuery ("PRAGMA user_version;");
		rs->next ();
		int32_t cur_version = rs->getInt (1);
		if (DATABASE_VERSION != cur_version)
			create_database (st);
	}

	~InstallerImpl ()
	{
		singleton_ = nullptr;
	}

	static void create_database (Statement::_ptr_type st)
	{
		st->executeUpdate (db_script);
	}

private:
	NDBC::Connection::_ref_type connection_;

	static InstallerImpl* singleton_;
};

InstallerImpl* InstallerImpl::singleton_ = nullptr;

class InstallerFactoryImpl : public CORBA::servant_traits <InstallerFactory>::ServantStatic <InstallerFactoryImpl>
{
public:
	static Installer::_ref_type get_installer ()
	{
		return InstallerImpl::get_singleton ();
	}
};

}

namespace CORBA {
namespace Internal {

template <>
const char StaticId < ::Nirvana::InstallerFactoryImpl>::id [] = "Nirvana/installer_factory";

}
}

NIRVANA_EXPORT_OBJECT (_exp_Nirvana_installer_factory, Nirvana::InstallerFactoryImpl)
