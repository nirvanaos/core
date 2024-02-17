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
#include "Connection.h"

namespace SQLite {

void deactivate_servant (PortableServer::Servant servant)
{
	PortableServer::POA::_ref_type adapter = servant->_default_POA ();
	adapter->deactivate_object (adapter->servant_to_id (servant));
}

NDBC::SQLWarning Connection::create_warning (int err) const
{
	return NDBC::SQLWarning (err, sqlite3_errmsg (sqlite_));
}

void Connection::add_warning (IDL::String&& msg)
{
	warnings_.emplace_back (0, std::move (msg));
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

void Connection::check_result (int err) const
{
	if (err)
		throw NDBC::SQLException (create_warning (err), NDBC::SQLWarnings ());
}

}
