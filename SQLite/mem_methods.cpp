/*
* Nirvana SQLite driver.
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
#include "mem_methods.h"
#include <Nirvana/c_heap_dbg.h>

using namespace Nirvana;

namespace SQLite {

void* xMalloc (int size) noexcept
{
	return c_malloc <HeapBlockHdrType> (alignof (std::max_align_t), size);
}

void xFree (void* p) noexcept
{
	c_free <HeapBlockHdrType> (p);
}

void* xRealloc (void* p, int size) noexcept
{
	return c_realloc <HeapBlockHdrType> (p, size);
}

int xSize (void* p) noexcept
{
	return (int)c_size <HeapBlockHdrType> (p);
}

int xRoundup (int size) noexcept
{
	if (size < std::numeric_limits <size_t>::max () / 4)
		size = (int)(clp2 (size + sizeof (HeapBlockHdrType)) - sizeof (HeapBlockHdrType));
	return size;
}

int xInit (void*) noexcept
{
	return SQLITE_OK;
}

void xShutdown (void*) noexcept
{}

const struct sqlite3_mem_methods mem_methods = {
	&xMalloc,
	&xFree,
	&xRealloc,
	&xSize,
	&xRoundup,
	&xInit,
	&xShutdown
};

}
