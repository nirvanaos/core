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
#ifndef NDBC_COLUMNINDEX_H_
#define NDBC_COLUMNINDEX_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC.h>
#include <Nirvana/string_conv.h>
#include <unordered_map>

namespace NDBC {

class ColumnIndex
{
	using ColumnMap = std::unordered_map <std::string, Ordinal>;
	using ColumnMapW = std::unordered_map <std::wstring, Ordinal>;

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

	void build (const NDBC::ColumnNames& column_names)
	{
		assert (empty ());

		bool non_ascii = false;
		for (const auto& column : column_names) {
			if (!is_ascii (column)) {
				non_ascii = true;
				break;
			}
		}

		if (!non_ascii) {
			new (&u_.narrow) ColumnMap ();
			d_ = Type::NARROW;

			Ordinal ord = 1;
			for (auto it = column_names.cbegin (); it != column_names.cend (); ++it, ++ord) {
				std::string lc = *it;
				std::transform (lc.begin (), lc.end (), lc.begin (), tolower_ascii);
				u_.narrow.emplace (lc, ord);
			}
		} else {
			new (&u_.wide) ColumnMapW ();
			d_ = Type::WIDE;

			Ordinal ord = 1;
			for (auto it = column_names.cbegin (); it != column_names.cend (); ++it, ++ord) {
				std::wstring lc = u2w (*it);
				std::transform (lc.begin (), lc.end (), lc.begin (), towlower);
				u_.wide.emplace (lc, ord);
			}
		}
	}

	const Ordinal* find (const IDL::String& name) const
	{
		switch (d_) {
		case Type::NARROW: {
			std::string lc = name;
			std::transform (lc.begin (), lc.end (), lc.begin (), tolower_ascii);
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
	static bool is_ascii (const IDL::String& s) noexcept
	{
		for (auto c : s) {
			if (c & 0x80)
				return false;
		}
		return true;
	}

	static int tolower_ascii (int c) noexcept
	{
		assert ((unsigned)c <= 127);
		if ('A' <= c && c <= 'Z')
			return c + 0x20;
		else
			return c;
	}

	static std::wstring u2w (const std::string& u)
	{
		std::wstring w;
		Nirvana::append_wide (u, w);
		return w;
	}

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

}

#endif
