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
#include <Nirvana/string_conv.h>

namespace SQLite {

class PreparedStatement final :
	public StatementBase,
	public CORBA::servant_traits <NDBC::PreparedStatement>::Servant <PreparedStatement>
{
public:
	PreparedStatement (Connection& conn, const IDL::String& sql, unsigned flags) :
		StatementBase (conn, this)
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

	void clearParameters ()
	{
		check_exist ();
		change_version ();
		for (auto stmt : statements ()) {
			sqlite3_clear_bindings (stmt);
		}
	}

	bool execute ()
	{
		check_exist ();
		return execute_first (true);
	}

	NDBC::ResultSet::_ref_type executeQuery ()
	{
		execute ();
		return getResultSet ();
	}

	void setBigInt (NDBC::Ordinal i, int64_t v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int64 (pi.stmt, pi.idx, v));
	}

	void setBigIntByName (const IDL::String& name, int64_t v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int64 (pi.stmt, pi.idx, v));
	}

	void setBlob (NDBC::Ordinal i, const NDBC::Blob& v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_blob64 (pi.stmt, pi.idx, v.data (),
			(sqlite3_uint64)v.size (), nullptr));
	}

	void setBlobByName (const IDL::String& name, const NDBC::Blob& v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_blob64 (pi.stmt, pi.idx, v.data (),
			(sqlite3_uint64)v.size (), nullptr));
	}

	void setBoolean (NDBC::Ordinal i, bool v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setBooleanByName (const IDL::String& name, bool v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setByte (NDBC::Ordinal i, uint8_t v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setByteByName (const IDL::String& name, uint8_t v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setDecimal (NDBC::Ordinal i, const CORBA::Any& v)
	{
		setDouble (i, fixed2double (v));
	}

	void setDecimalByName (const IDL::String& name, const CORBA::Any& v)
	{
		setDoubleByName (name, fixed2double (v));
	}

	void setDouble (NDBC::Ordinal i, CORBA::Double v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	void setDoubleByName (const IDL::String& name, CORBA::Double v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	void setFloat (NDBC::Ordinal i, float v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	void setFloatByName (const IDL::String& name, float v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_double (pi.stmt, pi.idx, v));
	}

	void setInt (NDBC::Ordinal i, int32_t v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setIntByName (const IDL::String& name, int32_t v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setNull (NDBC::Ordinal i)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_null (pi.stmt, pi.idx));
	}

	void setNullByName (const IDL::String& name)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_null (pi.stmt, pi.idx));
	}

	void setSmallInt (NDBC::Ordinal i, int16_t v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setSmallIntByName (const IDL::String& name, int16_t v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	void setString (NDBC::Ordinal i, const IDL::String& v)
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_text (pi.stmt, pi.idx, v.data (), (int)v.size (), nullptr));
	}

	void setStringByName (const IDL::String& name, const IDL::String& v)
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_text (pi.stmt, pi.idx, v.data (), (int)v.size (), nullptr));
	}

	void setWString (NDBC::Ordinal i, const IDL::WString& v)
	{
		IDL::String s;
		Nirvana::append_utf8 (v, s);
		setString (i, s);
	}

	void setWStringByName (const IDL::String& name, const IDL::WString& v)
	{
		IDL::String s;
		Nirvana::append_utf8 (v, s);
		setStringByName (name, s);
	}

private:
	ParamIndex get_param_index (unsigned i);
	ParamIndex get_param_index (const IDL::String & name);

	static double fixed2double (const CORBA::Any& v);
};

}

#endif
