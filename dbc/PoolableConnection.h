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
#include <unordered_set>

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

	ConnectionData ()
	{}

	ConnectionData (Connection::_ref_type&& conn) :
		Connection::_ref_type (std::move (conn))
	{}

	ConnectionData (const ConnectionData&) = delete;
	ConnectionData (ConnectionData&&) = default;

	ConnectionData& operator = (const ConnectionData&) = delete;
	ConnectionData& operator = (ConnectionData&&) = default;
};

class PoolableConnection : 
	public PoolableS <ConnectionData, Connection, PoolableConnection>
{
	using Base = PoolableS <ConnectionData, Connection, PoolableConnection>;

public:
	PoolableConnection (Pool <ConnectionData>& pool, ConnectionData&& cd, PortableServer::POA::_ptr_type adapter) :
		Base (pool, std::move (cd), adapter)
	{
		catalog_ = data ()->getCatalog ();
		schema_ = data ()->getSchema ();
		read_only_ = data ()->isReadOnly ();
		ti_ = data ()->getTransactionIsolation ();
	}

	void setTimeout (const TimeBase::TimeT&) const noexcept
	{}

	TimeBase::TimeT getTimeout () const noexcept
	{
		return 0;
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
		return CORBA::make_reference <PoolableStatement> (_this (), std::ref (pool), std::move (s), adapter ())->_this ();
	}

	PreparedStatement::_ref_type prepareStatement (const IDL::String& sql,
		ResultSet::Type resultSetType, unsigned flags)
	{
		auto& d = data ();
		RefPool <PreparedStatement>& pool = d.prepared_statements.emplace (sql,
			StatementPool <PreparedStatement> ()).first->second.types [(size_t)resultSetType];
		PreparedStatement::_ref_type s;
		if (!pool.empty ()) {
			s = std::move (pool.top ());
			pool.pop ();
		} else
			s = data ()->prepareStatement (sql, resultSetType, PreparedStatement::PREPARE_PERSISTENT);
		return CORBA::make_reference <PoolablePreparedStatement> (_this (), std::ref (pool), std::move (s), adapter ())->_this ();
	}

	SQLWarnings getWarnings ()
	{
		return data ()->getWarnings ();
	}

	void clearWarnings ()
	{
		data ()->clearWarnings ();
	}

	bool getAutoCommit ()
	{
		return data ()->getAutoCommit ();
	}

	void setAutoCommit (bool autoCommit)
	{
		data ()->setAutoCommit (autoCommit);
	}

	void commit ()
	{
		data ()->commit ();
	}

	IDL::String getCatalog ()
	{
		return data ()->getCatalog ();
	}

	void setCatalog (const IDL::String& catalog)
	{
		data ()->setCatalog (catalog);
	}

	IDL::String getSchema ()
	{
		return data ()->getSchema ();
	}

	void setSchema (const IDL::String& catalog)
	{
		data ()->setSchema (catalog);
	}

	TransactionIsolation getTransactionIsolation ()
	{
		return data ()->getTransactionIsolation ();
	}

	void setTransactionIsolation (TransactionIsolation level)
	{
		data ()->setTransactionIsolation (level);
	}

	bool isReadOnly ()
	{
		return data ()->isReadOnly ();
	}

	void setReadOnly (bool ro)
	{
		data ()->setReadOnly (ro);
	}

	Savepoint setSavepoint (const IDL::String& name)
	{
		Savepoint sp = data ()->setSavepoint (name);
		try {
			savepoints_.insert (sp);
		} catch (...) {
			try {
				data ()->releaseSavepoint (sp);
			} catch (...) {}
			throw;
		}
		return sp;
	}

	void releaseSavepoint (const Savepoint& savepoint)
	{
		data ()->releaseSavepoint (savepoint);
		savepoints_.erase (savepoint);
	}

	void rollback (const Savepoint& savepoint)
	{
		data ()->rollback (savepoint);
	}

	void cleanup (ConnectionData& data)
	{
		if (!savepoints_.empty ())
		for (auto it = savepoints_.cbegin (); it != savepoints_.cend (); ++it) {
			try {
				data->releaseSavepoint (*it);
			} catch (...) {
				assert (false);
			}
		}

		if (!data->getAutoCommit ()) {
			data->rollback (nullptr);
			data->setAutoCommit (true);
		}

		data->setTransactionIsolation (ti_);
		data->setCatalog (catalog_);
		data->setSchema (schema_);
		data->setReadOnly (read_only_);
	}

private:
	IDL::String catalog_, schema_;
	std::unordered_set <Savepoint> savepoints_;
	TransactionIsolation ti_;
	bool read_only_;
};

}

#endif
