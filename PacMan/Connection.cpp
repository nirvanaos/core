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
#include "Connection.h"
#include "version.h"

#define DATABASE_PATH "/var/lib/packages.db"

const char Connection::connect_rwc [] = "file:" DATABASE_PATH "?mode=rwc&journal_mode=WAL";
const char Connection::connect_rw [] = "file:" DATABASE_PATH "?mode=rw";
const char Connection::connect_ro [] = "file:" DATABASE_PATH "?mode=ro";

NDBC::Connection::_ref_type Connection::open_database (const char* url)
{
	return SQLite::driver->connect (url, nullptr, nullptr);
}

Connection::~Connection ()
{}

Connection::Statement Connection::get_statement (std::string sql)
{
	auto ins = statements_.emplace (std::move (sql), NDBC::PreparedStatement::_ref_type ());
	if (!ins.second) {
		try {
			cleanup (ins.first->second);
		} catch (...) {
			assert (false);
			ins.first->second = nullptr;
		}
	}
	if (!ins.first->second) {
		try {
			ins.first->second = connection_->prepareStatement (ins.first->first,
				NDBC::ResultSet::Type::TYPE_FORWARD_ONLY, NDBC::PreparedStatement::PREPARE_PERSISTENT);
		} catch (...) {
			statements_.erase (ins.first);
			throw;
		}
	}
	return ins.first->second;
}

void Connection::close_statements () noexcept
{
	Statements tmp (std::move (statements_));
	for (auto& st : tmp) {
		try {
			st.second->close ();
		} catch (...) {
			assert (false);
		}
	}
}

void Connection::cleanup (NDBC::PreparedStatement::_ptr_type st)
{
	st->clearParameters ();
}

NIRVANA_NORETURN void Connection::on_sql_exception (NDBC::SQLException& ex)
{
	Nirvana::BindError::Error err;
	NIRVANA_TRACE ("SQL error: %s\n", ex.error ().sqlState ().c_str ());
	Nirvana::BindError::set_message (err.info (), std::move (ex.error ().sqlState ()));
	for (NDBC::SQLWarning& sqle : ex.next ()) {
		NIRVANA_TRACE ("SQL error: %s\n", sqle.sqlState ().c_str ());
		Nirvana::BindError::set_message (Nirvana::BindError::push (err), std::move (sqle.sqlState ()));
	}
	throw err;
}

Nirvana::PM::Packages::Modules Connection::get_module_dependencies (Nirvana::ModuleId id)
{
	Nirvana::throw_NO_IMPLEMENT ();
}

Nirvana::PM::Packages::Modules Connection::get_dependent_modules (Nirvana::ModuleId)
{
	Nirvana::throw_NO_IMPLEMENT ();
}

void Connection::get_module_bindings (Nirvana::ModuleId id, Nirvana::PM::ModuleBindings& metadata)
{
	Statement stm = get_statement (
		"SELECT name,major,minor,interface,type FROM export WHERE module=? ORDER BY name,major,minor;"
		"SELECT name,version,interface FROM import WHERE module=? ORDER BY name,version,interface;"
	);
	stm->setInt (1, id);

	auto rs = stm->executeQuery ();

	while (rs->next ()) {
		metadata.exports ().emplace_back (
			Nirvana::PM::ObjBinding (rs->getString (1), rs->getSmallInt (2), rs->getSmallInt (3),
				rs->getString (4)),
			(Nirvana::PM::ObjType)rs->getInt (5));
	}

	NIRVANA_VERIFY (stm->getMoreResults ());
	rs = stm->getResultSet ();

	while (rs->next ()) {
		uint32_t ver = rs->getInt (2);
		metadata.imports ().emplace_back (rs->getString (1), major (ver), minor (ver),
			rs->getString (3));
	}
}

void Connection::get_binding (const IDL::String& name, Nirvana::PlatformId platform,
	Nirvana::PM::Binding& binding)
{
	const char* name_begin = name.data ();
	const char* sver = CORBA::Internal::RepId::version (name_begin, name_begin + name.size ());
	CORBA::Internal::RepId::Version version (sver);
	NDBC::ResultSet::_ref_type rs;
	try {
		Statement stm = get_statement ("SELECT id,path,platform,flags"
			" FROM module JOIN export ON export.module=module.id JOIN binary ON binary.module=module.id"
			" WHERE export.name=? AND platform=? AND major=? AND minor>=?");
		stm->setString (1, IDL::String (name_begin, sver - name_begin));
		stm->setInt (2, platform);
		stm->setInt (3, version.major);
		stm->setInt (4, version.minor);
		rs = stm->executeQuery ();
	} catch (const NDBC::SQLException& ex) {
		Nirvana::BindError::Error err;
		err.info ().s (ex.error ().sqlState ());
		err.info ()._d (Nirvana::BindError::Type::ERR_MESSAGE);
		throw err;
	}

	if (rs->next ()) {
		binding.module_id (rs->getInt (1));
		binding.binary_path (rs->getString (2));
		binding.platform (rs->getInt (3));
		binding.module_flags (rs->getSmallInt (4));
	} else {
		Nirvana::BindError::Error err;
		err.info ().s (name);
		err.info ()._d (Nirvana::BindError::Type::ERR_OBJ_NAME);
		throw err;
	}
}

IDL::String Connection::get_module_name (Nirvana::ModuleId id)
{
	Statement stm = get_statement ("SELECT name,version,prerelease FROM module WHERE id=?");
	stm->setInt (1, id);
	NDBC::ResultSet::_ref_type rs = stm->executeQuery ();
	IDL::String name;
	if (rs->next ()) {
		SemVer svname (rs->getString (1), rs->getBigInt (2), rs->getString (3));
		name = svname.to_string ();
	}
	return name;
}


