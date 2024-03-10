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
#include "../pch.h"
#include "IndirectMap.h"
#include "../Heap.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

IndirectMapMarshal::IndirectMapMarshal () :
	Base (Heap::user_heap ())
{}

std::pair <IndirectMapMarshal::iterator, bool> IndirectMapMarshal::emplace (uintptr_t key, uintptr_t val)
{
	return Base::emplace (key, val);
}

Internal::Interface* IndirectMapMarshal::find (uintptr_t pos) const noexcept
{
	auto it = Base::find (pos);
	if (it == end ())
		return nullptr;
	return (Internal::Interface*)it->second;
}

}
}
