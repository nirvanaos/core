/*
* Nirvana SQLite driver.
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
#include "Connection_impl.h"

namespace SQLite {

void deactivate_servant (PortableServer::Servant servant) noexcept
{
	try {
		PortableServer::POA::_ref_type adapter = servant->_default_POA ();
		// If shutdown is started, adapter will be nill reference
		if (adapter)
			adapter->deactivate_object (adapter->servant_to_id (servant));
	} catch (...) {
		assert (false);
	}
}

NIRVANA_NORETURN void throw_exception (IDL::String msg, int code)
{
	throw NDBC::SQLException (NDBC::SQLWarning (code, std::move (msg)), NDBC::SQLWarnings ());
}

NDBC::SQLWarning create_warning (sqlite3* conn, int err)
{
	return NDBC::SQLWarning (err, sqlite3_errmsg (conn));
}

NDBC::SQLException create_exception (sqlite3* conn, int err)
{
	return NDBC::SQLException (create_warning (conn, err), NDBC::SQLWarnings ());
}

void check_result (sqlite3* conn, int err)
{
	if (err)
		throw create_exception (conn, err);
}

void SQLite::close () noexcept
{
	if (sqlite_) {
		sqlite3_close_v2 (sqlite_);
		sqlite_ = nullptr;
	}
}

void Connection::check_exist () const
{
	if (isClosed ())
		throw CORBA::OBJECT_NOT_EXIST ();
}

void Connection::check_warning (int err) noexcept
{
	if (err) {
		try {
			warnings_.emplace_back (create_warning (err));
		} catch (...) {
		}
	}
}

void Connection::exec (const char* sql)
{
	Lock lock (*this);
	SQLite::exec (sql);
}

Connection::Lock::Lock (Connection& conn, bool no_check_exist) :
	connection_ (conn)
{
	if (!no_check_exist)
		conn.check_exist ();

	if (!conn.data_state_->inconsistent (conn.timeout_))
		throw CORBA::TRANSIENT ();
}

inline int Connection::busy_handler (int attempt) noexcept
{
	NIRVANA_TRACE ("SQLite: Connection is locked, attempt %d\n", attempt);
	if (attempt >= busy_retry_max_)
		return 0; // Stop attempts and throw SQLITE_BUSY
	else
		return 1;
}

int Connection::s_busy_handler (void* obj, int attempt) noexcept
{
	return ((Connection*)obj)->busy_handler (attempt);
}

}
