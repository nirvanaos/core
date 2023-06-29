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
	size_t size = nc.id ().size ();
	if (!nc.kind ().empty ())
		size += nc.kind ().size () + 1;
	Istring name;
	name.reserve (size);
	name = escape (nc.id ());
	if (!nc.kind ().empty ()) {
		name += '.';
		name += escape (nc.kind ());
	}
	return name;
}

Istring NamingContextRoot::to_string (NameComponent&& nc)
{
	Istring name (escape (std::move (nc.id ())));
	if (!nc.kind ().empty ()) {
		name.reserve (name.size () + nc.kind ().size () + 1);
		name += '.';
		name += escape (std::move (nc.kind ()));
	}
	return name;
}

NameComponent NamingContextRoot::to_component (Istring s)
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
			nc.id (unescape (std::move (s)));
	}
	if (id_size < s_size) {
		if (id_size > 0)
			nc.kind (unescape (s.substr (id_size + 1)));
		else {
			s.erase (0, 1);
			nc.kind (unescape (std::move (s)));
		}
	}
	return nc;
}

void NamingContextRoot::check_name (const Name& n) const
{
	if (destroyed ())
		throw CORBA::OBJECT_NOT_EXIST ();

	if (n.empty ())
		throw NamingContext::InvalidName ();
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

void NamingContextRoot::destroy ()
{
	if (destroyed ())
		throw CORBA::OBJECT_NOT_EXIST ();

	destroyed_ = true;
}

void NamingContextRoot::list (uint32_t how_many, BindingList& bl, 
	CosNaming::BindingIterator::_ref_type& bi) const
{
	if (destroyed_)
		throw CORBA::OBJECT_NOT_EXIST ();

	auto vi = make_iterator ();
	vi->next_n (how_many, bl);
	if (!vi->end ())
		bi = Iterator::create_iterator (std::move (vi));
}

}
}
