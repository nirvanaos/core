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
#include <Nirvana/Hash.h>
#include "../Source/MapUnorderedStable.h"

#define PS_PARAM(name, type)\
void set##name (NDBC::Ordinal param, type v) { setParam (get_param_index (param), v); } \
void set##name##ByName (const IDL::String& param, type v) { setParam (get_param_index (param), v); }

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

	void clearParameters ()
	{
		Connection::Lock lock (connection ());
		change_version ();
		for (auto stmt : statements ()) {
			sqlite3_clear_bindings (stmt);
			sqlite3_reset (stmt);
		}
		param_storage_.clear ();
	}

	bool execute ()
	{
		Connection::Lock lock (connection ());
		return execute_first (true);
	}

	NDBC::ResultSet::_ref_type executeQuery ()
	{
		execute ();
		return getResultSet ();
	}

	uint32_t executeUpdate ()
	{
		Connection::Lock lock (connection ());
		return StatementBase::executeUpdate ();
	}

	PS_PARAM (BigInt, int64_t);
	PS_PARAM (Blob, NDBC::Blob&);
	PS_PARAM (Boolean, bool);
	PS_PARAM (Byte, CORBA::Octet);
	PS_PARAM (Decimal, const CORBA::Any&);
	PS_PARAM (Double, CORBA::Double);
	PS_PARAM (Float, CORBA::Float);
	PS_PARAM (Int, int32_t);
	PS_PARAM (SmallInt, int16_t);
	PS_PARAM (String, IDL::String&);
	PS_PARAM (NString, const IDL::WString&);

	void setNull (NDBC::Ordinal param)
	{
		setParam (get_param_index (param));
	}

	void setNullByName (const IDL::String& param)
	{
		setParam (get_param_index (param));
	}

	void close ()
	{
		StatementBase::close ();
		param_storage_.clear ();
	}

private:
	struct ParamIndex
	{
		unsigned stmt;
		unsigned param;

		bool operator == (const ParamIndex& rhs) const noexcept
		{
			return stmt == rhs.stmt && param == rhs.param;
		}
	};

	struct ParamHash
	{
		size_t operator () (const ParamIndex& pi) const
		{
			return Nirvana::Hash::hash_bytes (&pi, sizeof (pi));
		}
	};

	void setParam (const ParamIndex& pi, int64_t v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_int64 (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, NDBC::Blob& v)
	{
		Connection::Lock lock (connection ());
		const auto& p = save_param (pi, v);
		lock.connection ().check_result (sqlite3_bind_blob (statements () [pi.stmt], pi.param,
			p.data (), (int)p.size (), SQLITE_STATIC));
	}

	void setParam (const ParamIndex& pi, bool v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_int (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, uint8_t v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_int (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, const CORBA::Any& v)
	{
		setParam (pi, fixed2double (v));
	}

	void setParam (const ParamIndex& pi, CORBA::Double v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_double (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, float v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_double (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, int32_t v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_int (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_null (statements () [pi.stmt], pi.param));
	}

	void setParam (const ParamIndex& pi, int16_t v)
	{
		Connection::Lock lock (connection ());
		lock.connection ().check_result (sqlite3_bind_int (statements () [pi.stmt], pi.param, v));
	}

	void setParam (const ParamIndex& pi, IDL::String& v)
	{
		Connection::Lock lock (connection ());
		const auto& p = save_param (pi, v);
		lock.connection ().check_result (sqlite3_bind_text (statements () [pi.stmt], pi.param,
			p.data (), (int)p.size (), SQLITE_STATIC));
	}

	void setParam (const ParamIndex& pi, const IDL::WString& v)
	{
		IDL::String s;
		Nirvana::append_utf8 (v, s);
		setParam (pi, s);
	}

	ParamIndex get_param_index (unsigned i);
	ParamIndex get_param_index (const IDL::String & name);

	static double fixed2double (const CORBA::Any& v);

	class ParamStorage
	{
	public:
		ParamStorage () noexcept :
			d_ (EMPTY)
		{}

		ParamStorage (const ParamStorage&) = delete;

		ParamStorage (ParamStorage&& src) noexcept
		{
			adopt (src);
		}

		~ParamStorage ()
		{
			destruct ();
		}

		ParamStorage& operator = (const ParamStorage& src) = delete;

		ParamStorage& operator = (ParamStorage&& src) noexcept
		{
			destruct ();
			adopt (src);
			return *this;
		}

		const IDL::String& set (IDL::String& v) noexcept
		{
			destruct ();
			construct (u_.string, std::move (v));
			d_ = STRING;
			return u_.string;
		}

		const NDBC::Blob& set (NDBC::Blob& v) noexcept
		{
			destruct ();
			construct (u_.blob, std::move (v));
			d_ = BLOB;
			return u_.blob;
		}

	private:
		template <class T>
		static void destruct (T& v) noexcept
		{
			v.~T ();
		}

		template <class T>
		static void construct (T& v, T&& src) noexcept
		{
			new (&v) T (std::move (src));
		}

		void destruct () noexcept;
		void adopt (ParamStorage& src) noexcept;

		enum D
		{
			EMPTY,
			STRING,
			BLOB
		} d_;

		union U {
			U ()
			{}
			~U ()
			{}

			IDL::String string;
			NDBC::Blob blob;
		} u_;
	};

	ParamStorage& param_storage (const ParamIndex& pi);

	template <class T>
	const T& save_param (const ParamIndex& pi, T& v)
	{
		if (v.empty ())
			return v;
		else
			return param_storage (pi).set (v);
	}

private:
	Nirvana::Core::MapUnorderedStable <ParamIndex, ParamStorage, ParamHash> param_storage_;
};

}

#undef PS_PARAM

#endif
