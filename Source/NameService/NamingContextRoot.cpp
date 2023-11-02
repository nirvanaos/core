/*
* Nirvana Core.
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
#include "NamingContextRoot.h"
#include "Iterator.h"

namespace CosNaming {
namespace Core {

void NamingContextRoot::append_string (Istring& s, const NameComponent& nc)
{
	size_t size = nc.id ().size ();
	if (!nc.kind ().empty ())
		size += nc.kind ().size () + 1;
	s.reserve (s.size () + size);
	append_escaped (s, nc.id (), "\\/");
	if (!nc.kind ().empty ()) {
		s += '.';
		append_escaped (s, nc.kind (), "\\/.");
	}
}

void NamingContextRoot::append_string (Istring& s, NameComponent&& nc)
{
	if (nc.kind ().empty ())
		append_escaped (s, std::move (nc.id ()), "\\/");
	else
		append_string (s, const_cast <const NameComponent&> (nc));
}

NameComponent NamingContextRoot::to_component (Istring& s, bool may_move)
{
	const size_t s_size = s.size ();
	const char* s_p = s.data ();
	size_t id_size = s_size;
	for (;;) {
		size_t point = s.rfind ('.', id_size);
		if (Istring::npos == point)
			break;
		if (point > 0 && s_p [point - 1] == '\\') {
			// Escaped
			id_size = point - 2;
		} else {
			id_size = point;
			break;
		}
	}

	NameComponent nc;
	if (id_size > 0) {

		// A trailing '.' character is not permitted by the specification.
		if (id_size == s_size - 1)
			throw NamingContext::InvalidName ();

		if (id_size < s_size)
			nc.id (unescape (s.substr (0, id_size)));
		else
			nc.id (unescape (may_move ? std::move (s) : s));
	}
	if (id_size < s_size) {
		if (id_size > 0)
			nc.kind (unescape (s.substr (id_size + 1)));
		else {
			s.erase (0, 1);
			nc.kind (unescape (may_move ? std::move (s) : s));
		}
	}
	return nc;
}

void NamingContextRoot::check_name (const Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
}

void NamingContextRoot::append_escaped (Istring& dst, const Istring& src, const char* escape_chars)
{
	for (size_t pos = 0; pos < src.size ();) {
		size_t esc = src.find_first_of (escape_chars, pos);
		if (esc != Istring::npos) {
			dst.append (src, pos, esc - pos);
			dst.insert (esc, 1, '\\');
			pos = esc + 1;
		} else {
			dst.append (src, pos, Istring::npos);
			break;
		}
	}
}

void NamingContextRoot::append_escaped (Istring& dst, Istring&& src, const char* escape_chars)
{
	if (dst.empty () && src.find_first_of (escape_chars) == Istring::npos)
		dst = std::move (src);
	else
		append_escaped (dst, const_cast <const Istring&> (src), escape_chars);
}

Istring NamingContextRoot::unescape (Istring s)
{
	for (size_t pos = 0; (pos = s.find ('\\', pos)) != Istring::npos;) {
		s.erase (pos, 1);
		++pos;
	}
	return s;
}

void NamingContextRoot::list (uint32_t how_many, BindingList& bl, 
	CosNaming::BindingIterator::_ref_type& bi) const
{
	auto vi = make_iterator ();
	vi->next_n (how_many, bl);
	if (!vi->end ())
		bi = Iterator::create_iterator (std::move (vi));
}

}
}
