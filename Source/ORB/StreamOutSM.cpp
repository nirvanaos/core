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
#include "StreamOutSM.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Internal {
namespace Core {

void StreamOutSM::write (size_t align, size_t size, const void* buf, size_t allocated_size)
{
}

void StreamOutSM::allocate_block (size_t cb)
{
	blocks_.emplace_back ();
	Block& block = blocks_.back ();
	block.ptr = Port::Memory::allocate (nullptr, cb, 0);
	block.size = cb;
}

}
}
}
