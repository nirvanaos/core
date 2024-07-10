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
#ifndef NIRVANA_PACMAN_CONNECTION_H_
#define NIRVANA_PACMAN_CONNECTION_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC.h>

#define DATABASE_PATH "/var/lib/packages.db"

class Connection
{
public:
	Connection ()
	{}

	Connection (NDBC::Connection::_ref_type&& conn) noexcept :
		connection_ (std::move (conn))
	{}

	void connect (NDBC::Connection::_ref_type&& conn) noexcept
	{
		connection_ = std::move (conn);
	}

	void get_binding (const IDL::String& name, Nirvana::PlatformId platform,
		Nirvana::Packages::Binding& binding)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

	IDL::String get_module_name (Nirvana::ModuleId id)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

	Nirvana::Packages::Modules get_module_dependencies (Nirvana::ModuleId id)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

	Nirvana::Packages::Modules get_dependent_modules (Nirvana::ModuleId)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

protected:
	static NDBC::Connection::_ref_type open_database (const char* url)
	{
		return SQLite::driver->connect (url, nullptr, nullptr);
	}

	static NDBC::Connection::_ref_type open_rwc ()
	{
		return open_database ("file:" DATABASE_PATH "?mode=rwc&journal_mode=WAL");
	}

	static NDBC::Connection::_ref_type open_ro ()
	{
		return open_database ("file:" DATABASE_PATH "?mode=ro");
	}

	static NDBC::Connection::_ref_type open_rw ()
	{
		return open_database ("file:" DATABASE_PATH "?mode=rw");
	}

	void commit ()
	{
		connection_->commit ();
		connection_->close ();
		connection_ = nullptr;
	}

	void rollback ()
	{
		connection_->rollback (nullptr);
		connection_->close ();
		connection_ = nullptr;
	}

	struct SemVer
	{
		uint64_t version;
		std::string name;
		std::string prerelease;

		SemVer (const std::string& mod_name);
	};

private:
	NDBC::Connection::_ref_type connection_;
};

#endif
