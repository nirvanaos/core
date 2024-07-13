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
#ifndef PACMAN_CONNECTION_H_
#define PACMAN_CONNECTION_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC.h>
#include <unordered_map>
#include "SemVer.h"

class Connection
{
public:
	static const char connect_rwc [];
	static const char connect_rw [];
	static const char connect_ro [];

	static NDBC::Connection::_ref_type open_database (const char* url);

	Connection (const char* connstr) :
		connection_ (open_database (connstr))
	{}

	~Connection ();

	typedef NDBC::PreparedStatement::_ptr_type Statement;

	void get_binding (const IDL::String& name, Nirvana::PlatformId platform,
		Nirvana::PM::Binding& binding);

	IDL::String get_module_name (Nirvana::ModuleId id);

	Nirvana::PM::Packages::Modules get_module_dependencies (Nirvana::ModuleId id);
	Nirvana::PM::Packages::Modules get_dependent_modules (Nirvana::ModuleId id);

	void get_module_bindings (Nirvana::ModuleId id, Nirvana::PM::ModuleBindings& metadata);

protected:
	Statement get_statement (std::string sql);

	static NIRVANA_NORETURN void on_sql_exception (NDBC::SQLException& ex);

	void commit ()
	{
		try {
			close_statements ();
			connection_->commit ();
			connection_->close ();
			connection_ = nullptr;
		} catch (NDBC::SQLException& ex) {
			on_sql_exception (ex);
		}
	}

	void rollback ()
	{
		try {
			close_statements ();
			connection_->rollback (nullptr);
			connection_->close ();
			connection_ = nullptr;
		} catch (NDBC::SQLException& ex) {
			on_sql_exception (ex);
		}
	}

	NDBC::Connection::_ptr_type connection () const noexcept
	{
		return connection_;
	}

	static uint16_t major (uint32_t v) noexcept
	{
		return (uint16_t)(v >> 16);
	}

	static uint16_t minor (uint32_t v) noexcept
	{
		return (uint16_t)v;
	}

	static int32_t version (uint16_t major, uint16_t minor) noexcept
	{
		return ((int32_t)major << 16) | minor;
	}

private:
	void close_statements ();

private:
	typedef std::unordered_map <std::string, NDBC::PreparedStatement::_ref_type> Statements;
	NDBC::Connection::_ref_type connection_;
	Statements statements_;
};

#endif
