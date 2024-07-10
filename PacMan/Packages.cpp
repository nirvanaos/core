/*
* Nirvana package manager.
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

#define DATABASE_VERSION 1

#define STRINGIZE0(n) #n
#define STRINGIZE(n) STRINGIZE0 (n)

const char* const Packages::db_script_ [] = {

"BEGIN;"

// Modules
"CREATE TABLE module("
"id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL,version INTEGER NOT NULL,"
"prerelease TEXT,flags INTEGER NOT NULL,"
"UNIQUE (name,version,prerelease)"
");",

// Module binaries
"CREATE TABLE binary("
"module INTEGER NOT NULL REFERENCES module(id)ON DELETE CASCADE,"
"platform INTEGER NOT NULL,path TEXT NOT NULL,"
"UNIQUE(module,platform)"
");"

// Module imports and exports
"CREATE TABLE object("
"name TEXT NOT NULL,version INTEGER NOT NULL,"
"module INTEGER NOT NULL REFERENCES module(id)ON DELETE CASCADE,flags INTEGER NOT NULL,"
// For the each particular object:version, the specific module may import or export it but not both.
// If the module exports and imports the same object, then only export must be recorded to database.
"UNIQUE(name,version,module)"
");"

// Reserve first 100 module identifiers as the system
"INSERT INTO module(id, name, version, flags)VALUES(" STRINGIZE (MAX_SYS_MODULE_ID) ",'',0,0); "
"DELETE FROM module WHERE id=" STRINGIZE (MAX_SYS_MODULE_ID) ";"

// Set database version
"PRAGMA user_version=" STRINGIZE (DATABASE_VERSION) ";"
"COMMIT;"
};

Packages::Packages (CORBA::Object::_ptr_type component) :
	Servant (component)
{
	create_database ();
	connect (open_ro ());
}

inline void Packages::create_database ()
{
	NDBC::Connection::_ref_type connection = open_rwc ();
	NDBC::Statement::_ref_type st = connection->createStatement (NDBC::ResultSet::Type::TYPE_FORWARD_ONLY);
	NDBC::ResultSet::_ref_type rs = st->executeQuery ("PRAGMA user_version;");
	rs->next ();
	int32_t cur_version = rs->getInt (1);
	if (DATABASE_VERSION != cur_version) {
		for (const char* sql : db_script_) {
#ifndef NDEBUG
			try {
#endif
				st->executeUpdate (sql);
#ifndef NDEBUG
			} catch (const NDBC::SQLException& ex) {
				NIRVANA_TRACE ("Error: %s\n", ex.error ().sqlState ().c_str ());
				NIRVANA_TRACE ("SQL: %s\n", sql);
				throw;
			}
#endif
		}
	}
}

