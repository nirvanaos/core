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
#include "VirtualIterator.h"

namespace CosNaming {
namespace Core {

NameComponent VirtualIterator::to_component (const Istring& s)
{
	NameComponent nc;
	size_t dot = s.rfind ('.');
	if (dot == Istring::npos)
		nc.id (s);
	else {
		nc.id (s.substr (0, dot));
		nc.kind (s.substr (dot + 1));
	}
	return nc;
}

bool VirtualIterator::next_one (CosNaming::Binding& b)
{
	Binding vb;
	if (next_one (vb)) {
		b.binding_name ().push_back (to_component (vb.name));
		b.binding_type (vb.type);
		return true;
	}
	return false;
}

bool VirtualIterator::next_n (uint32_t how_many, CosNaming::BindingList& bl)
{
	CosNaming::Binding b;
	bool ret = false;
	while (next_one (b)) {
		bl.push_back (std::move (b));
		ret = true;
	}
	return ret;
}

}
}

