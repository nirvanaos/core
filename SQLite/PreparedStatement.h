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
#ifndef SQLITE_PREPAREDSTATEMENT_H_
#define SQLITE_PREPAREDSTATEMENT_H_
#pragma once

#include "StatementBase.h"

namespace SQLite {

class PreparedStatement final :
	public virtual CORBA::servant_traits <NDBC::PreparedStatement>::base_type,
	public virtual StatementBase
{
public:
	PreparedStatement (Connection& conn, const IDL::String& sql, unsigned flags) :
		StatementBase (conn)
	{
		prepare (sql, flags);
	}

	~PreparedStatement ()
	{}

	struct ParamIndex
	{
		sqlite3_stmt* stmt;
		unsigned idx;
	};

	virtual void clearParameters () override;
	
	virtual bool execute () override;
	virtual NDBC::ResultSet::_ref_type executeQuery () override;
	virtual int32_t executeUpdate () override;

	virtual void setBigInt (NDBC::Ordinal i, int64_t v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int64 (pi.stmt, pi.idx, v));
	}

	virtual void setBigIntByName (const IDL::String& name, int64_t v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int64 (pi.stmt, pi.idx, v));
	}

	virtual void setBlob (NDBC::Ordinal i, const NDBC::BLOB& v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_blob64 (pi.stmt, pi.idx, v.data (),
			(sqlite3_uint64)v.size (), nullptr));
	}

	virtual void setBlobByName (const IDL::String& name, const NDBC::BLOB& v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_blob64 (pi.stmt, pi.idx, v.data (),
			(sqlite3_uint64)v.size (), nullptr));
	}

	virtual void setBoolean (NDBC::Ordinal i, bool v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setBooleanByName (const IDL::String& name, bool v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setByte (NDBC::Ordinal i, uint8_t v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setByteByName (const IDL::String& name, uint8_t v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setDecimal (NDBC::Ordinal i, const CORBA::Any& v) override
	{
		setDouble (i, fixed2double (v));
	}

	virtual void setDecimalByName (const IDL::String& name, const CORBA::Any& v) override
	{
		setDoubleByName (name, fixed2double (v));
	}

	virtual void setDouble (NDBC::Ordinal i, CORBA::Double v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	virtual void setDoubleByName (const IDL::String& name, CORBA::Double v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	virtual void setFloat (NDBC::Ordinal i, float v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	virtual void setFloatByName (const IDL::String& name, float v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	virtual void setInt (NDBC::Ordinal i, int32_t v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setIntByName (const IDL::String& name, int32_t v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setNull (NDBC::Ordinal i) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_null (pi.stmt, pi.idx));
	}

	virtual void setNullByName (const IDL::String& name) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_null (pi.stmt, pi.idx));
	}

	virtual void setSmallInt (NDBC::Ordinal i, int16_t v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setSmallIntByName (const IDL::String& name, int16_t v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setString (NDBC::Ordinal i, const IDL::String& v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_text (pi.stmt, pi.idx, v.data (), (int)v.size (), nullptr));
	}

	virtual void setStringByName (const IDL::String& name, const IDL::String& v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_text (pi.stmt, pi.idx, v.data (), (int)v.size (), nullptr));
	}

private:
	ParamIndex get_param_index (unsigned i);
	ParamIndex get_param_index (const IDL::String & name);

	static double fixed2double (const CORBA::Any& v);
};

}

#endif
