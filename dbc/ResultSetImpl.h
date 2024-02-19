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
		Base::pageMaxCount (fetch_max_count);
		Base::pageMaxSize (fetch_max_size);
	}

	ResultSetImpl ()
	{}

	~ResultSetImpl ();

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
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void close ()
	{
		check ();
		Base::cache ().clear ();
		Base::column_names ().clear ();
		Base::position (0);
		Base::column_count (0);
		Base::cache_position (0);
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

	CORBA::Any getDecimal (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	Blob getBlob (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool getBoolean (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Octet getByte (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Double getDouble (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
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
		return position ();
	}

	int16_t getSmallInt (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	StatementBase::_ref_type getStatement ()
	{
		check ();
		return cursor ()->getStatement ();
	}

	IDL::String getString (Ordinal columnIndex)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	SQLWarnings getWarnings () const
	{
		check ();
		return SQLWarnings ();
	}

	bool isAfterLast ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool isBeforeFirst ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool isClosed ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool isFirst ()
	{
		throw CORBA::NO_IMPLEMENT ();
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
		throw CORBA::NO_IMPLEMENT ();
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
	void check () const
	{
		if (!cursor ())
			throw_exception ("ResultSet is closed");
	}

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

	static std::wstring u2w (const IDL::String& utf8);

	const Variant& field (Ordinal ord) const;

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
