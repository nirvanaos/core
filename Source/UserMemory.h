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
#ifndef NIRVANA_CORE_USERMEMORY_H_
#define NIRVANA_CORE_USERMEMORY_H_

#include <CORBA/Server.h>
#include <generated/Memory_s.h>
#include "user_memory.h"

namespace Nirvana {
namespace Core {

/// Delegates memory operations dependent on context.
class UserMemory :
	public ::CORBA::Internal::ServantStatic <UserMemory, Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t size, unsigned flags)
	{
		return user_memory ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return user_memory ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return user_memory ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return user_memory ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t size, unsigned flags)
	{
		return user_memory ().copy (dst, src, size, flags);
	}

	static bool is_readable (const void* p, size_t size)
	{
		return user_memory ().is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return user_memory ().is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return user_memory ().is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return user_memory ().is_copy (p1, p2, size);
	}

	static intptr_t query (const void* p, Memory::QueryParam q)
	{
		return user_memory ().query (p, q);
	}
};

}
}

#endif
