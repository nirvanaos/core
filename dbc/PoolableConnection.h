/*
* Database connection module.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NDBC_POOLABLECONNECTION_H_
#define NDBC_POOLABLECONNECTION_H_
#pragma once

#include "PoolableStatement.h"
#include <unordered_map>
#include <Nirvana/Hash.h>

namespace NDBC {

template <class I>
struct StatementPool
{
	RefPool <I> types [(size_t)ResultSet::Type::TYPE_SCROLL_SENSITIVE + 1];
};

struct ConnectionData : public Connection::_ref_type
{
	StatementPool <Statement> statements;
	std::unordered_map <std::string, StatementPool <PreparedStatement> > prepared_statements;

	ConnectionData (Connection::_ref_type&& conn) :
		Connection::_ref_type (std::move (conn))
	{}

	ConnectionData (const ConnectionData&) = delete;
	ConnectionData (ConnectionData&&) = default;

	ConnectionData& operator = (const ConnectionData&) = delete;
	ConnectionData& operator = (ConnectionData&&) = default;
};

class PoolableConnection : 
	public CORBA::servant_traits <Connection>::Servant <PoolableConnection>,
	public PoolableS <ConnectionData, PoolableConnection>
{
	using Base = PoolableS <ConnectionData, PoolableConnection>;

public:
	PoolableConnection (Pool <ConnectionData>& pool, ConnectionData&& data) :
		Base (pool, std::move (data))
	{}

	bool isValid (uint32_t sec)
	{
		return data ()->isValid (sec);
	}

	Statement::_ref_type createStatement (ResultSet::Type resultSetType)
	{
		auto& d = data ();
		RefPool <Statement>& pool = d.statements.types [(size_t)resultSetType];
		Statement::_ref_type s;
		if (!pool.empty ()) {
			s = std::move (pool.top ());
			pool.pop ();
		} else
			s = data ()->createStatement (resultSetType);
		return CORBA::make_reference <PoolableStatement> (_this (), std::ref (pool), std::move (s))->_this ();
	}

	PreparedStatement::_ref_type prepareStatement (const IDL::String& sql,
		ResultSet::Type resultSetType, unsigned flags)
	{
		auto& d = data ();
		RefPool <PreparedStatement>& pool = d.prepared_statements.emplace (sql,
			StatementPool <PreparedStatement> ()).first->second [(size_t)resultSetType];
		PreparedStatement::_ref_type s;
		if (!pool.empty ()) {
			s = std::move (pool.top ());
			pool.pop ();
		} else
			s = data ()->prepareStatement (sql, resultSetType, PreparedStatement::PREPARE_PERSISTENT);
		return CORBA::make_reference <PoolablePreparedStatement> (_this (), std::ref (pool), std::move (s))->_this ();
	}
};

}

#endif
