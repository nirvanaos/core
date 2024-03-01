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
#ifndef SQLITE_CONNECTION_IMPL_H_
#define SQLITE_CONNECTION_IMPL_H_
#pragma once

#include "Statement.h"
#include "PreparedStatement.h"

namespace SQLite {

inline
NDBC::Statement::_ref_type Connection::createStatement (NDBC::ResultSet::Type resultSetType)
{
	if (resultSetType != NDBC::ResultSet::Type::TYPE_FORWARD_ONLY)
		throw_exception ("Unsupported ResultSet type");
	return CORBA::make_reference <Statement> (std::ref (*this))->_this ();
}

inline
NDBC::PreparedStatement::_ref_type Connection::prepareStatement (const IDL::String& sql, NDBC::ResultSet::Type resultSetType, unsigned flags)
{
	if (resultSetType != NDBC::ResultSet::Type::TYPE_FORWARD_ONLY)
		throw_exception ("Unsupported ResultSet type");
	return CORBA::make_reference <PreparedStatement> (std::ref (*this), sql, flags)->_this ();
}

}

#endif
