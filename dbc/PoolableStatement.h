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
#ifndef NDBC_POOLABLESTATEMENT_H_
#define NDBC_POOLABLESTATEMENT_H_
#pragma once

#include "Pool.h"

namespace NDBC {

class PoolableStatementBase
{
public:
	bool isClosed () const
	{
		return !base_;
	}

	bool getMoreResults () const
	{
		return base ()->getMoreResults ();
	}

	long getUpdateCount () const
	{
		return base ()->getUpdateCount ();
	}

	SQLWarnings getWarnings () const
	{
		return base ()->getWarnings ();
	}

	void clearWarnings () const
	{
		base ()->clearWarnings ();
	}

	ResultSet::Type getResultSetType () const
	{
		return base ()->getResultSetType ();
	}

	Connection::_ref_type getConnection () const
	{
		return conn_;
	}

protected:
	PoolableStatementBase (Connection::_ref_type&& conn, StatementBase::_ptr_type base) noexcept;

	void close () noexcept;

private:
	StatementBase::_ptr_type base () const;

private:
	Connection::_ref_type conn_;
	StatementBase::_ptr_type base_;
};

template <class I>
using RefPool = Pool <typename I::ref_type>;

template <class I, class S>
class PoolableStatementS :
	public PoolableStatementBase,
	public PoolableS <typename I::_ref_type, S>,
	public CORBA::servant_traits <I>::Servant <S>
{
	using Base = PoolableS <typename I::_ref_type, S>;
	using Servant = CORBA::servant_traits <I>::Servant <S>;

public:
	using PoolType = Base::PoolType;

	using PoolableStatementBase::isClosed;

	ResultSet::_ref_type getResultSet ()
	{
		ResultSet::_ref_type rs = base ()->getResultSet ();
		rs->statement (Servant::_this ());
		return rs;
	}

	void close ()
	{
		Base::close ();
		PoolableStatementBase::close ();
	}

protected:
	PoolableStatementS (Connection::_ref_type&& conn, PoolType& pool, typename I::_ref_type&& impl)
		noexcept :
		PoolableStatementBase (std::move (conn), impl),
		Base (pool, std::move (impl))
	{}

};

class PoolableStatement : public PoolableStatementS <Statement, PoolableStatement>
{
	using Base = PoolableStatementS <Statement, PoolableStatement>;
	using PoolType = Base::PoolType;
	using DataType = Base::DataType;

public:
	PoolableStatement (Connection::_ref_type&& conn, PoolType& pool, Statement::_ref_type&& impl)
		noexcept :
		Base (std::move (conn), pool, std::move (impl))
	{}
};

class PoolablePreparedStatement : public PoolableStatementS <PreparedStatement,
	PoolablePreparedStatement>
{
	using Base = PoolableStatementS <PreparedStatement, PoolablePreparedStatement>;

public:
	PoolablePreparedStatement (Connection::_ref_type&& conn, PoolType& pool,
		PreparedStatement::_ref_type&& impl) noexcept :
		Base (std::move (conn), pool, std::move (impl))
	{}
};

}

#endif
