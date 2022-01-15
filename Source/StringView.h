/// \file
/*
* Nirvana Core. Windows port library.
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
#ifndef NIRVANA_CORE_PORT_STRINGVIEW_H_
#define NIRVANA_CORE_PORT_STRINGVIEW_H_
#pragma once

#include <Nirvana/Nirvana.h>

namespace Nirvana {
namespace Core {

/// Strings are passed to Port as StringView.
class StringView
{
public:
	template <class Al>
	StringView (const std::basic_string <char, std::char_traits <char>, Al>& s) :
		c_str_ (s.c_str ()),
		length_ (s.length ())
	{}

	StringView (const char* s) :
		c_str_ (s),
		length_ (-1)
	{}

	const char* c_str () const
	{
		return c_str_;
	}

	const char* data () const
	{
		return c_str_;
	}
	
	size_t length () const
	{
		if (length_ < 0)
			length_ = strlen (c_str_);
		return length_;
	}

	size_t size () const
	{
		return length ();
	}

private:
	const char* c_str_; //< Zero-terminated string
	mutable ptrdiff_t length_; //< String length or -1 if unknown
};

}
}
#endif
