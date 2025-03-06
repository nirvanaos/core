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

class PoolableConnection;

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

	Connection::_ref_type getConnection () const;

protected:
	PoolableStatementBase (PoolableConnection& conn, StatementBase::_ptr_type base) noexcept;
	~PoolableStatementBase ();

	void close () noexcept;

	StatementBase::_ptr_type base () const
	{
		if (!base_)
			PoolableBase::throw_closed ();
		return base_;
	}

	static void cleanup (StatementBase::_ptr_type s);

private:
	CORBA::servant_reference <PoolableConnection> conn_;
	StatementBase::_ptr_type base_;
};

template <class I>
using RefPool = Pool <typename I::_ref_type>;

template <class I, class S>
class PoolableStatementS :
	public PoolableStatementBase,
	public PoolableS <typename I::_ref_type, I, S>
{
	using Base = PoolableS <typename I::_ref_type, I, S>;

public:
	using PoolType = Base::PoolType;

	using PoolableStatementBase::isClosed;

	ResultSet::_ref_type getResultSet ()
	{
		ResultSet::_ref_type rs = base ()->getResultSet ();
		rs->statement (Base::_this ());
		return rs;
	}

	void close ()
	{
		Base::close ();
		PoolableStatementBase::close ();
	}

	static void cleanup (StatementBase::_ptr_type s)
	{
		PoolableStatementBase::cleanup (s);
	}

protected:
	PoolableStatementS (PoolableConnection& conn, PoolType& pool, typename I::_ref_type&& impl)
		noexcept :
		PoolableStatementBase (conn, impl),
		Base (pool, std::move (impl))
	{}

	~PoolableStatementS ()
	{
		Base::destruct ();
	}
};

class PoolableStatement : public PoolableStatementS <Statement, PoolableStatement>
{
	using Base = PoolableStatementS <Statement, PoolableStatement>;
	using PoolType = Base::PoolType;
	using DataType = Base::DataType;

public:
	PoolableStatement (PoolableConnection& conn, PoolType& pool, Statement::_ref_type&& impl)
		noexcept :
		Base (conn, pool, std::move (impl))
	{}

	bool execute (const IDL::String& sql)
	{
		return data ()->execute (sql);
	}

	ResultSet::_ref_type executeQuery (const IDL::String& sql)
	{
		ResultSet::_ref_type rs = data ()->executeQuery (sql);
		rs->statement (_this ());
		return rs;
	}

	uint32_t executeUpdate (const IDL::String& sql)
	{
		return data ()->executeUpdate (sql);
	}

};

class PoolablePreparedStatement : public PoolableStatementS <PreparedStatement,
	PoolablePreparedStatement>
{
	using Base = PoolableStatementS <PreparedStatement, PoolablePreparedStatement>;

public:
	PoolablePreparedStatement (PoolableConnection& conn, PoolType& pool,
		PreparedStatement::_ref_type&& impl) noexcept :
		Base (conn, pool, std::move (impl))
	{}

	void setBigInt (Ordinal idx, int64_t v)
	{
		data ()->setBigInt (idx, v);
	}

	void setBigIntByName (const IDL::String& name, int64_t v)
	{
		data ()->setBigIntByName (name, v);
	}

	void setBlob (Ordinal idx, const Blob& v)
	{
		data ()->setBlob (idx, v);
	}

	void setBlobByName (const IDL::String& name, const Blob& v)
	{
		data ()->setBlobByName (name, v);
	}

	void setBoolean (Ordinal idx, bool v)
	{
		data ()->setBoolean (idx, v);
	}

	void setBooleanByName (const IDL::String& name, bool v)
	{
		data ()->setBooleanByName (name, v);
	}

	void setByte (Ordinal idx, uint8_t v)
	{
		data ()->setByte (idx, v);
	}

	void setByteByName (const IDL::String& name, uint8_t v)
	{
		data ()->setByteByName (name, v);
	}

	void setDecimal (Ordinal idx, const CORBA::Any& v)
	{
		data ()->setDecimal (idx, v);
	}

	void setDecimalByName (const IDL::String& name, const CORBA::Any& v)
	{
		data ()->setDecimalByName (name, v);
	}

	void setDouble (Ordinal idx, double v)
	{
		data ()->setDouble (idx, v);
	}

	void setDoubleByName (const IDL::String& name, double v)
	{
		data ()->setDoubleByName (name, v);
	}

	void setFloat (Ordinal idx, float v)
	{
		data ()->setFloat (idx, v);
	}

	void setFloatByName (const IDL::String& name, float v)
	{
		data ()->setFloatByName (name, v);
	}

	void setInt (Ordinal idx, int32_t v)
	{
		data ()->setInt (idx, v);
	}

	void setIntByName (const IDL::String& name, int32_t v)
	{
		data ()->setIntByName (name, v);
	}

	void setNull (Ordinal idx)
	{
		data ()->setNull (idx);
	}

	void setNullByName (const IDL::String& name)
	{
		data ()->setNullByName (name);
	}

	void setSmallInt (Ordinal idx, int16_t v)
	{
		data ()->setSmallInt (idx, v);
	}

	void setSmallIntByName (const IDL::String& name, int16_t v)
	{
		data ()->setSmallIntByName (name, v);
	}

	void setString (Ordinal idx, const IDL::String& v)
	{
		data ()->setString (idx, v);
	}

	void setStringByName (const IDL::String& name, const IDL::String& v)
	{
		data ()->setStringByName (name, v);
	}

	void setNString (Ordinal idx, const IDL::WString& v)
	{
		data ()->setNString (idx, v);
	}

	void setNStringByName (const IDL::String& name, const IDL::WString& v)
	{
		data ()->setNStringByName (name, v);
	}

	bool execute ()
	{
		return data ()->execute ();
	}

	ResultSet::_ref_type executeQuery ()
	{
		ResultSet::_ref_type rs = data ()->executeQuery ();
		rs->statement (_this ());
		return rs;
	}

	uint32_t executeUpdate ()
	{
		return data ()->executeUpdate ();
	}

	void clearParameters ()
	{
		data ()->clearParameters ();
	}

	static void cleanup (PreparedStatement::_ref_type& s)
	{
		s->clearParameters ();
		Base::cleanup (s);
	}
};

}

#endif
