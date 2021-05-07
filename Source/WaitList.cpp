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
#include "WaitList.h"

using namespace std;

namespace Nirvana {
namespace Core {

void WaitList::start_construction ()
{
	if (state_ != State::INITIAL)
		throw CORBA::BAD_INV_ORDER ();
	state_ = State::UNDER_CONSTRUCTION;
}

void WaitList::end_construction ()
{
	assert (State::UNDER_CONSTRUCTION == state_);
	state_ = State::CONSTRUCTED;
}

}
}
