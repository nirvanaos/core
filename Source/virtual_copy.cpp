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
#include "pch.h"
#include "virtual_copy.h"
#include <Port/Memory.h>
#include <Nirvana/real_copy.h>

namespace Nirvana {
namespace Core {

void virtual_copy (const void* src, size_t size, void* dst, unsigned flags)
{
	if (!Port::Memory::SHARING_UNIT || size < Port::Memory::SHARING_UNIT
		||
		(uintptr_t)dst % Port::Memory::SHARING_ASSOCIATIVITY != (uintptr_t)src % Port::Memory::SHARING_ASSOCIATIVITY
	)
		real_move (src, (const void*)((const uint8_t*)src + size), dst);
	else
		Port::Memory::copy (dst, const_cast <void*> (src), size, flags);
}

}
}
