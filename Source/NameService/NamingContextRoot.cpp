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
	name = nc.id ();
	if (!nc.kind ().empty ()) {
		name += '.';
		name += nc.kind ();
	}
	return name;
}

Istring NamingContextRoot::to_string (NameComponent&& nc)
{
	Istring name (std::move (nc.id ()));
	if (!nc.kind ().empty ()) {
		name.reserve (name.size () + nc.kind ().size () + 1);
		name += '.';
		name += nc.kind ();
	}
	return name;
}

NameComponent NamingContextRoot::to_component (Istring s)
{
	const size_t s_size = s.size ();
	size_t id_size = s.rfind ('.');
	if (Istring::npos == id_size)
		id_size = s.size ();

	NameComponent nc;
	if (id_size > 0) {

		// A trailing '.' character is not permitted by the specification.
		if (id_size == s_size - 1)
			throw NamingContext::InvalidName ();

		if (id_size < s_size)
			nc.id (s.substr (0, id_size));
		else
			nc.id (std::move (s));
	}
	if (id_size < s_size) {
		if (id_size > 0)
			nc.kind (s.substr (id_size + 1));
		else {
			s.erase (1, 1);
			nc.kind (std::move (s));
		}
	}
	return nc;
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
