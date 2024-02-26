/*
* Database connection module.
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
#ifndef NDBC_RESULTSETIMPL_H_
#define NDBC_RESULTSETIMPL_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "ColumnIndex.h"
#include <Nirvana/NDBC_s.h>
#include <time.h>
#include <Nirvana/Formatter.h>

namespace NDBC {

class ResultSetImpl : public IDL::traits <ResultSet>::Servant <ResultSetImpl>
{
	typedef IDL::traits <ResultSet>::Servant <ResultSetImpl> Base;

public:
	ResultSetImpl (Ordinal column_count, uint_fast16_t flags, Cursor::_ptr_type cursor, const Row& init_row)
	{
		Base::column_count (column_count);
		Base::flags (flags);
		Base::cursor (cursor);
		if (init_row.empty ()) {
			// Empty rowset
			Base::position (1);
			Base::after_last (true);
		} else
			Base::row (init_row);
	}

	ResultSetImpl ()
	{}

	~ResultSetImpl ();

	bool absolute (RowOff pos)
	{
		if (!pos) {
			beforeFirst ();
			return !row ().empty ();
		} else
			return fetch (pos);
	}

	void afterLast ()
	{
		check_scrollable ();
		if (!empty ()) {
			position (0);
			after_last (true);
			row ().clear ();
		}
	}

	void beforeFirst ()
	{
		check_scrollable ();
		if (!empty ()) {
			position (0);
			after_last (false);
			row ().clear ();
		}
	}

	void clearWarnings ()
	{}

	void close ()
	{
		check ();
		Base::row ().clear ();
		Base::column_names ().clear ();
		Base::position (0);
		Base::column_count (0);
		Base::cursor (nullptr);
	}

	Ordinal findColumn (const IDL::String& columnLabel)
	{
		check ();
		const ColumnNames& names = column_names ();
		if (flags () & FLAG_COLUMNS_CASE_SENSITIVE) {
			auto it = std::find (names.begin (), names.end (), columnLabel);
			if (it != names.end ())
				return (Ordinal)(it - names.begin () + 1);
		} else {
			if (column_index_.empty ())
				column_index_.build (names);

			const Ordinal* pf = column_index_.find (columnLabel);
			if (pf)
				return *pf;
		}
		throw_exception ("Column not found: " + columnLabel);
	}

	bool first ()
	{
		return fetch (1);
	}

	Ordinal getColumnCount () const noexcept
	{
		check ();
		return column_count ();
	}

	int64_t getBigInt (Ordinal columnIndex)
	{
		return getBigInt (field (columnIndex));
	}

	Blob getBlob (Ordinal columnIndex)
	{
		return getBlob (field (columnIndex));
	}

	bool getBoolean (Ordinal columnIndex)
	{
		return getBoolean (field (columnIndex));
	}

	CORBA::Octet getByte (Ordinal columnIndex)
	{
		return getByte (field (columnIndex));
	}

	CORBA::Any getDecimal (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Double getDouble (Ordinal columnIndex)
	{
		return getDouble (field (columnIndex));
	}

	CORBA::Float getFloat (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	int32_t getInt (Ordinal columnIndex)
	{
		return getInt (field (columnIndex));
	}

	RowIdx getRow () const noexcept
	{
		check ();
		if (after_last ())
			return 0;
		else
			return position ();
	}

	int16_t getSmallInt (Ordinal columnIndex)
	{
		return getSmallInt (field (columnIndex));
	}

	StatementBase::_ref_type getStatement ()
	{
		check ();
		return cursor ()->getStatement ();
	}

	IDL::String getString (Ordinal columnIndex)
	{
		return getString (field (columnIndex));
	}

	IDL::WString getWString (Ordinal columnIndex)
	{
		IDL::WString ws;
		Nirvana::append_wide (getString (columnIndex), ws);
		return ws;
	}

	RSType getType () const
	{
		check ();
		if (flags () & FLAG_FORWARD_ONLY)
			return RSType::TYPE_FORWARD_ONLY;
		else if (flags () & FLAG_SCROLL_SENSITIVE)
			return RSType::TYPE_SCROLL_SENSITIVE;
		else
			return RSType::TYPE_SCROLL_INSENSITIVE;
	}

	SQLWarnings getWarnings () const
	{
		check ();
		return SQLWarnings ();
	}

	bool isAfterLast () const noexcept
	{
		return after_last ();
	}

	bool isBeforeFirst () const noexcept
	{
		return position () == 0;
	}

	bool isClosed () const noexcept
	{
		return !cursor ();
	}

	bool isFirst ()
	{
		return position () == 1 && !after_last ();
	}

	bool isLast ()
	{
		check_scrollable ();
		if (position () == 0)
			return false;

		if (!last_row ()) {
			Row tmp;
			last_row (cursor ()->fetch (-1, tmp));
		}

		return position () == last_row ();
	}

	bool last ()
	{
		return fetch (-1);
	}

	bool next ()
	{
		return fetch (0);
	}

	bool previous ()
	{
		check_scrollable ();
		RowIdx pos = position ();
		switch (pos) {
		case 0:
			return false;
		case 1:
			if (!after_last ()) {
				position (0);
				row ().clear ();
			}
			return false;

		default:
			if (after_last ())
				return fetch (-1);
			else
				return fetch (pos - 1);
		}
	}

	bool relative (RowOff rows)
	{
		if (empty ())
			return false;

		switch (rows) {
		case 0:
			return !row ().empty ();

		case 1:
			return next ();

		case -1:
			return previous ();

		default:
			check_scrollable ();
			if (rows > 0) {
				RowOff pos = (RowOff)position () + rows;
				if (pos <= 0) {
					afterLast ();
					return false;
				} else
					return fetch (pos);
			} else {
				RowOff pos = (RowOff)position () - rows;
				if (pos <= 0) {
					beforeFirst ();
					return false;
				} else
					return fetch (pos);
			}
		}
	}

private:
	const ColumnNames& column_names ()
	{
		check ();
		if (Base::column_names ().empty ())
			Base::column_names (cursor ()->getColumnNames ());
		return Base::column_names ();
	}

	NIRVANA_NORETURN static void throw_exception (IDL::String msg);
	NIRVANA_NORETURN static void data_conversion_error ();

	void check () const;
	void check_row_valid () const;
	void check_scrollable () const;

	bool empty () const noexcept
	{
		return 1 == position () && after_last ();
	}

	bool fetch (RowOff i);

	const Variant& field (Ordinal ord) const;

	static int64_t getBigInt (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return 0;
		case DB_DATE:
		case DB_INT:
			return v.l_val ();
		case DB_SMALLINT:
			return v.si_val ();
		case DB_TINYINT:
			return v.byte_val ();
		case DB_BIGINT:
			return v.ll_val ();
		}

		data_conversion_error ();
	}

	static int32_t getInt (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return 0;
		case DB_DATE:
		case DB_INT:
			return v.l_val ();
		case DB_SMALLINT:
			return v.si_val ();
		case DB_TINYINT:
			return v.byte_val ();
		case DB_BIGINT: {
			int64_t ll = v.ll_val ();
			if (std::numeric_limits <int32_t>::min () <= ll && ll <= std::numeric_limits <int32_t>::max ())
				return (int32_t)ll;
		} break;
		}

		data_conversion_error ();
	}

	static int32_t getSmallInt (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return 0;
		case DB_INT: {
			int32_t l = v.l_val ();
			if (std::numeric_limits <int16_t>::min () <= l && l <= std::numeric_limits <int16_t>::max ())
				return (int16_t)l;
		} break;
		case DB_SMALLINT:
			return v.si_val ();
		case DB_TINYINT:
			return v.byte_val ();
		case DB_BIGINT: {
			int64_t ll = v.ll_val ();
			if (std::numeric_limits <int16_t>::min () <= ll && ll <= std::numeric_limits <int16_t>::max ())
				return (int16_t)ll;
		} break;
		}

		data_conversion_error ();
	}

	static uint8_t getByte (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return 0;
		case DB_INT: {
			int32_t l = v.l_val ();
			if (std::numeric_limits <int8_t>::min () <= l && l <= std::numeric_limits <int8_t>::max ())
				return (int8_t)l;
		} break;
		case DB_SMALLINT: {
			int16_t si = v.si_val ();
			if (std::numeric_limits <int8_t>::min () <= si && si <= std::numeric_limits <int8_t>::max ())
				return (int8_t)si;
		} break;
		case DB_TINYINT:
			return v.byte_val ();
		case DB_BIGINT: {
			int64_t ll = v.ll_val ();
			if (std::numeric_limits <int8_t>::min () <= ll && ll <= std::numeric_limits <int8_t>::max ())
				return (int8_t)ll;
		} break;
		}

		data_conversion_error ();
	}

	static IDL::String getString (const Variant& v)
	{
		IDL::String ret;
		switch (v._d ()) {
		case DB_STRING:
			ret = v.s_val ();
			break;

		case DB_NULL:
			ret = "NULL";
			break;
		case DB_BIGINT:
			ret = std::to_string (v.ll_val ());
			break;
		case DB_DATE: {
			struct tm tminfo;
			time_t time = ((time_t)v.l_val () * TimeBase::DAY - TimeBase::UNIX_EPOCH) / TimeBase::SECOND;
			gmtime_r (&time, &tminfo);
			char buffer [16];
			strftime (buffer, sizeof (buffer), "%Y-%m-%d", &tminfo);
			ret = buffer;
		} break;
		case DB_DATETIME: {
			struct tm tminfo;
			time_t time = ((time_t)v.ll_val () - TimeBase::UNIX_EPOCH) / TimeBase::SECOND;
			gmtime_r (&time, &tminfo);
			char buffer [64];
			strftime (buffer, sizeof (buffer), "%Y-%m-%d %H:%M:%S", &tminfo);
			ret = buffer;
		} break;

		case DB_DOUBLE:
			ret = std::to_string (v.dbl_val ());
			break;
		case DB_FLOAT:
			ret = std::to_string (v.flt_val ());
			break;
		case DB_INT:
			ret = std::to_string (v.l_val ());
			break;
		case DB_SMALLINT:
			ret = std::to_string (v.si_val ());
			break;
		case DB_TINYINT:
			ret = std::to_string (v.byte_val ());
			break;
		case DB_TIME: {
			uint32_t s = (uint32_t)(v.ll_val () / TimeBase::SECOND);
			unsigned m = s / 60;
			s %= 60;
			unsigned h = m / 60;
			m %= 60;
			Nirvana::append_format (ret, "%02u:%02u:%02u", h, m, s);
		} break;

		default:
			data_conversion_error ();
		}

		return ret;
	}

	static Blob getBlob (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return Blob ();
		case DB_BINARY:
			return v.blob ();
		}

		data_conversion_error ();
	}

	static bool getBoolean (const Variant& v)
	{
		switch (v._d ()) {
		case DB_BIGINT:
			return v.ll_val () != 0;
		case DB_INT:
			return v.l_val () != 0;
		case DB_SMALLINT:
			return v.si_val () != 0;
		case DB_TINYINT:
			return v.byte_val () != 0;
		}

		data_conversion_error ();
	}

	static double getDouble (const Variant& v)
	{
		switch (v._d ()) {
		case DB_NULL:
			return 0;
		case DB_INT:
			return v.l_val ();
		case DB_SMALLINT:
			return v.si_val ();
		case DB_TINYINT:
			return v.byte_val ();
		case DB_DOUBLE:
			return v.dbl_val ();
		case DB_FLOAT:
			return v.flt_val ();
		}

		data_conversion_error ();
	}

private:

	ColumnIndex column_index_;
};

}

#endif
