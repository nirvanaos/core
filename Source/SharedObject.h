/// \file
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
#ifndef NIRVANA_CORE_SHAREDOBJECT_H_
#define NIRVANA_CORE_SHAREDOBJECT_H_
#pragma once

#include "MemContext.h"

namespace Nirvana {
namespace Core {

/// \brief Object allocated from the shared memory context.
class SharedObject
{
public:
	void* operator new (size_t cb)
	{
		return g_shared_mem_context->heap ().allocate (nullptr, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		g_shared_mem_context->heap ().release (p, cb);
	}

	void* operator new (size_t cb, void* place)
	{
		return place;
	}

	void operator delete (void*, void*)
	{}
};

}
}

#endif