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
#include <Nirvana/NDBC_s.h>
#include <unordered_map>
#include <time.h>
#include <Nirvana/Formatter.h>

namespace NDBC {

class ResultSetImpl : public IDL::traits <ResultSet>::Servant <ResultSetImpl>
{
	typedef IDL::traits <ResultSet>::Servant <ResultSetImpl> Base;

public:
	ResultSetImpl (Ordinal column_count, uint_fast16_t flags, Cursor::_ptr_type cursor, const Rows& rows, uint32_t fetch_max_count, uint32_t fetch_max_size)
	{
		Base::column_count (column_count);
		Base::flags (flags);
		Base::cursor (cursor);
		Base::cache (rows);
		Base::row (1);
		Base::after_last (rows.empty ());
		Base::pageMaxCount (fetch_max_count);
		Base::pageMaxSize (fetch_max_size);
	}

	ResultSetImpl ()
	{}

	~ResultSetImpl ();

	uint32_t pageMaxCount () const noexcept
	{
		return Base::pageMaxCount ();
	}

	void pageMaxCount (uint32_t v)
	{
		if (!v)
			throw CORBA::BAD_PARAM ();
		Base::pageMaxCount (v);
	}

	bool absolute (uint32_t row)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void afterLast ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void beforeFirst ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void clearWarnings ()
	{}

	void close ()
	{
		check ();
		Base::cache ().clear ();
		Base::column_names ().clear ();
		Base::row (0);
		Base::column_count (0);
		Base::cache_row (0);
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
			if (column_index_.empty ()) {
				if (flags () & FLAG_COLUMNS_NON_ASCII)
					column_index_.build_wide (names);
				else
					column_index_.build_narrow (names);
			}

			const Ordinal* pf = column_index_.find (columnLabel);
			if (pf)
				return *pf;
		}
		throw_exception ("Column not found: " + columnLabel);
	}

	bool first ()
	{
		throw CORBA::NO_IMPLEMENT ();
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

	uint32_t getRow () const noexcept
	{
		check ();
		return row ();
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
		return row () == 0;
	}

	bool isClosed () const noexcept
	{
		return !cursor ();
	}

	bool isFirst ()
	{
		return row () == 1 && !after_last ();
	}

	bool isLast ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool last ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool next ()
	{
		check ();
		uint32_t next_cached = Base::cache_row () + 1;
		if (!after_last ()) {
			uint32_t next_row = Base::row () + 1;
			if (next_cached >= cache ().size ()) {
				Rows rows = cursor ()->getNext (next_row, Base::pageMaxCount (), Base::pageMaxSize ());
				if (rows.empty ())
					Base::after_last (true);
				if (!rows.empty () || (flags () & FLAG_FORWARD_ONLY)) {
					cache (std::move (rows));
					Base::cache_row (0);
					next_cached = 0;
				}
			} else if (flags () & FLAG_FORWARD_ONLY)
				cache ().erase (cache ().begin ());
			else
				Base::cache_row (next_cached);
			Base::row (next_row);
		}

		return !after_last ();
	}

	bool previous ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool relative (int32_t rows)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

private:
	void check () const;
	void check_row_valid () const;

	static bool is_ascii (const IDL::String& s) noexcept
	{
		for (auto c : s) {
			if (c & 0x80)
				return false;
		}
		return true;
	}

	const ColumnNames& column_names ()
	{
		check ();
		if (Base::column_names ().empty ()) {
			Base::column_names (cursor ()->getColumnNames ());
			if (!(flags () & FLAG_COLUMNS_NON_ASCII)) {
				for (const auto& name : column_names ()) {
					if (!is_ascii (name)) {
						flags () |= FLAG_COLUMNS_NON_ASCII;
						break;
					}
				}
			}
		}
		return Base::column_names ();
	}

	NIRVANA_NORETURN static void throw_exception (IDL::String msg);
	NIRVANA_NORETURN static void data_conversion_error ();
	NIRVANA_NORETURN static void error_forward_only ();

	static std::wstring u2w (const IDL::String& utf8);

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
	using ColumnMap = std::unordered_map <std::string, Ordinal>;
	using ColumnMapW = std::unordered_map <std::wstring, Ordinal>;

	class ColumnIndex
	{
	public:
		ColumnIndex () :
			d_ (Type::EMPTY)
		{}

		~ColumnIndex ()
		{
			switch (d_) {
			case Type::NARROW:
				destruct (u_.narrow);
				break;
			case Type::WIDE:
				destruct (u_.wide);
				break;
			}
		}

		ColumnIndex (const ColumnIndex& src) :
			d_ (Type::EMPTY)
		{
			switch (d_) {
			case Type::NARROW:
				new (&u_.narrow) ColumnMap (src.u_.narrow);
				break;
			case Type::WIDE:
				new (&u_.wide) ColumnMapW (src.u_.wide);
				break;
			}
			d_ = src.d_;
		}

		bool empty () const noexcept
		{
			return Type::EMPTY == d_;
		}

		void build_narrow (const ColumnNames& names)
		{
			assert (Type::EMPTY == d_);
			new (&u_.narrow) ColumnMap ();
			d_ = Type::NARROW;

			Ordinal ord = 1;
			for (auto it = names.begin (); it != names.end (); ++it, ++ord) {
				std::string lc = *it;
				std::transform (lc.begin (), lc.end (), lc.begin (), tolower);
				u_.narrow.emplace (lc, ord);
			}
		}

		void build_wide (const ColumnNames& names)
		{
			assert (Type::EMPTY == d_);
			new (&u_.wide) ColumnMapW ();
			d_ = Type::WIDE;

			Ordinal ord = 1;
			for (auto it = names.begin (); it != names.end (); ++it, ++ord) {
				std::wstring lc = u2w (*it);
				std::transform (lc.begin (), lc.end (), lc.begin (), towlower);
				u_.wide.emplace (lc, ord);
			}
		}

		const Ordinal* find (const IDL::String& name) const
		{
			switch (d_) {
			case Type::NARROW: {
				std::string lc = name;
				std::transform (lc.begin (), lc.end (), lc.begin (), tolower);
				auto it = u_.narrow.find (lc);
				if (it != u_.narrow.end ())
					return &it->second;
			} break;

			case Type::WIDE: {
				std::wstring lc = u2w (name);
				std::transform (lc.begin (), lc.end (), lc.begin (), towlower);
				auto it = u_.wide.find (lc);
				if (it != u_.wide.end ())
					return &it->second;
			} break;

			default:
				NIRVANA_UNREACHABLE_CODE ();
			}

			return nullptr;
		}

	private:
		template <class T>
		static void destruct (T& v) noexcept
		{
			v.~T ();
		}

		enum class Type {
			EMPTY,
			NARROW,
			WIDE
		};

		union U {
			U ()
			{}

			~U ()
			{}

			ColumnMap narrow;
			ColumnMapW wide;
		} u_;

		Type d_;
	};

	ColumnIndex column_index_;
};

}

#endif
