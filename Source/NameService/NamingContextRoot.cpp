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

Istring NamingContextRoot::to_string (const NameComponent& nc)
{
	Istring name = escape (nc.id ());
	name += '.';
	name += escape (nc.kind ());
	return name;
}

Istring NamingContextRoot::to_string (NameComponent&& nc)
{
	Istring name = escape (std::move (nc.id ()));
	name += '.';
	name += escape (std::move (nc.kind ()));
	return name;
}

NameComponent NamingContextRoot::to_component (Istring s)
{
	NameComponent nc;
	size_t s_size = s.size ();
	size_t id_size = s_size;
	for (;;) {
		size_t dot = s.rfind ('.', id_size);
		if (Istring::npos == dot) {
			id_size = s.size ();
			break;
		} else if (dot > 0 && s [dot - 1] == '\\') // escaped
			id_size = dot - 1;
		else {
			id_size = dot;
			break;
		}
	}

	if (id_size > 0) {
		Istring id;
		if (id_size < s_size)
			id = s.substr (0, id_size);
		else
			id = std::move (s);
		nc.id (unescape (std::move (id)));
	}
	if (id_size < s_size) {
		Istring kind;
		if (id_size > 0)
			kind = s.substr (id_size + 1);
		else {
			s.erase (1, 1);
			kind = std::move (s);
		}
		nc.kind (unescape (std::move (kind)));
	}
	return nc;
}

Istring NamingContextRoot::escape (Istring s)
{
	for (size_t pos = 0; pos < s.size ();) {
		size_t esc = s.find_first_of ("/.\\", 0);
		if (esc != Istring::npos) {
			s.insert (esc, 1, '\\');
			pos = esc + 2;
		} else
			break;
	}
	return s;
}

Istring NamingContextRoot::unescape (Istring s)
{
	for (size_t pos = 0; (pos = s.find ('\\', pos)) != Istring::npos;) {
		s.erase (pos, 1);
		++pos;
	}
	return s;
}

void NamingContextRoot::check_name (const Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
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
