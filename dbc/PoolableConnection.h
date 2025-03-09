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
#include "../Source/MapUnorderedStable.h"
#include "../Source/MapUnorderedUnstable.h"

namespace NDBC {

template <class I>
struct StatementPool
{
	RefPool <I> types [(size_t)ResultSet::Type::TYPE_SCROLL_SENSITIVE + 1];

	void clear () noexcept
	{
		for (auto& pool : types) {
			pool.clear ();
		}
	}
};

struct ConnectionData : public Connection::_ref_type
{
	ConnectionData ()
	{}

	ConnectionData (Connection::_ref_type&& conn) :
		Connection::_ref_type (std::move (conn)),
		catalog (p_->getCatalog ()),
		schema (p_->getSchema ()),
		ti (p_->getTransactionIsolation ()),
		read_only (p_->isReadOnly ())
	{}

	void reset () const
	{
		p_->setTransactionIsolation (ti);
		p_->setCatalog (catalog);
		p_->setSchema (schema);
		p_->setReadOnly (read_only);
	}

	ConnectionData (const ConnectionData&) = delete;
	ConnectionData (ConnectionData&&) = default;

	ConnectionData& operator = (const ConnectionData&) = delete;
	ConnectionData& operator = (ConnectionData&&) = delete;

	StatementPool <Statement> statements;
	Nirvana::Core::MapUnorderedStable <std::string, StatementPool <PreparedStatement> > prepared_statements;
	IDL::String catalog, schema;
	Connection::TransactionIsolation ti;
	bool read_only;
};

class ConnectionPoolImpl;

class PoolableConnection : 
	public PoolableS <ConnectionData, Connection, PoolableConnection>
{
	using Base = PoolableS <ConnectionData, Connection, PoolableConnection>;

public:
	PoolableConnection (ConnectionPoolImpl& pool, ConnectionData&& cd);

	~PoolableConnection ()
	{
		destruct ();
	}

	void setTimeout (const TimeBase::TimeT& t)
	{
		data ()->setTimeout (t);
	}

	TimeBase::TimeT getTimeout ()
	{
		return data ()->getTimeout ();
	}

	Statement::_ref_type createStatement (ResultSet::Type resultSetType)
	{
		ConnectionData& d = data ();
		RefPool <Statement>& pool = d.statements.types [(size_t)resultSetType];
		Statement::_ref_type s;
		if (!pool.empty ()) {
			s = std::move (pool.top ());
			pool.pop ();
		} else
			s = d->createStatement (resultSetType);
		return CORBA::make_reference <PoolableStatement> (std::ref (*this), std::ref (pool),
			std::move (s))->_this ();
	}

	PreparedStatement::_ref_type prepareStatement (const IDL::String& sql,
		ResultSet::Type resultSetType, unsigned flags)
	{
		ConnectionData& d = data ();
		RefPool <PreparedStatement>& pool = d.prepared_statements.emplace (sql,
			StatementPool <PreparedStatement> ()).first->second.types [(size_t)resultSetType];
		PreparedStatement::_ref_type s;
		if (!pool.empty ()) {
			s = std::move (pool.top ());
			pool.pop ();
		} else
			s = d->prepareStatement (sql, resultSetType, PreparedStatement::PREPARE_PERSISTENT);
		return CORBA::make_reference <PoolablePreparedStatement> (std::ref (*this), std::ref (pool),
			std::move (s))->_this ();
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

	void close ()
	{
		if (PoolableBase::close ()) {
			if (0 == active_statements_)
				release_to_pool ();
			Base::deactivate (this);
		}
	}

	void add_active_statement () noexcept
	{
		assert (!isClosed ());
		++active_statements_;
	}

	void remove_active_statement () noexcept
	{
		if ((0 == --active_statements_) && isClosed ())
			release_to_pool ();
	}

	void release_to_pool () noexcept;
	void cleanup (ConnectionData& data);

private:
	CORBA::servant_reference <ConnectionPoolImpl> parent_;
	Nirvana::Core::SetUnorderedUnstable <Savepoint> savepoints_;
	unsigned active_statements_;
};

inline Connection::_ref_type PoolableStatementBase::getConnection () const
{
	return conn_->_this ();
}

}

#endif
