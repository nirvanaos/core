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
#include "IteratorStack.h"

namespace CosNaming {
namespace Core {

bool IteratorStack::next_one (Binding& b)
{
	if (!stack_.empty ()) {
		b = std::move (stack_.back ());
		stack_.pop_back ();
		return true;
	}
	return false;
}

void IteratorStack::push (const Istring& name, BindingType type)
{
	stack_.emplace_back (name, type);
}

void IteratorStack::push (Istring&& name, BindingType type)
{
	stack_.emplace_back (std::move (name), type);
}


}
}
