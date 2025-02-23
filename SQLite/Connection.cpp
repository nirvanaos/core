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
#include <Nirvana/POSIX.h>

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
	assert (attempt == 0 || attempt == last_busy_cnt_ + 1);

	if (attempt >= (int)BUSY_TRY_MAX)
		return 0; // Stop attempts and throw SQLITE_BUSY

	unsigned pow;
	if (attempt == 0) {
		if (last_busy_cnt_ == 0) {
			if (busy_timeout_pow_ > 0)
				--busy_timeout_pow_;
		} else {
			unsigned newpow = busy_timeout_pow_ + last_busy_cnt_ / 2;
			if (newpow > BUSY_TIMEOUT_POW_MAX)
				newpow = BUSY_TIMEOUT_POW_MAX;
			busy_timeout_pow_ = newpow;
		}
		pow = busy_timeout_pow_;
	} else {
		pow = busy_timeout_pow_ + attempt;
		if (pow > BUSY_TIMEOUT_POW_MAX)
			pow = BUSY_TIMEOUT_POW_MAX;
	}
	last_busy_cnt_ = attempt;

	TimeBase::TimeT tsleep = BUSY_TIMEOUT_MIN << pow;
	tsleep += ((BUSY_TIMEOUT_MIN * Nirvana::the_posix->rand ()) >> (sizeof (unsigned) * 8))
		- BUSY_TIMEOUT_MIN / 2;
	assert (tsleep < (BUSY_TIMEOUT_MIN << (BUSY_TIMEOUT_POW_MAX + 1)));
	Nirvana::the_posix->sleep (tsleep);
	return 1;
}

int Connection::s_busy_handler (void* obj, int attempt) noexcept
{
	return ((Connection*)obj)->busy_handler (attempt);
}

}
