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
#include <CORBA/Server.h>
#include <IDL/Memory_s.h>
#include <ExecDomain.h>

namespace Nirvana {
namespace Core {

/// Implementation of the Nirvana::Memory interface.
class Memory :
	public CORBA::servant_traits <Nirvana::Memory>::ServantStatic <Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t& size, unsigned flags)
	{
		return MemContext::current ().heap ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return MemContext::current ().heap ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return MemContext::current ().heap ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return MemContext::current ().heap ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t& size, unsigned flags)
	{
		return MemContext::current ().heap ().copy (dst, src, size, flags);
	}

	static bool is_private (const void* p, size_t size)
	{
		return MemContext::current ().heap ().is_private (p, size);
	}

	static intptr_t query (const void* p, Memory::QueryParam q)
	{
		return MemContext::current ().heap ().query (p, q);
	}
};

}

__declspec (selectany)
const ImportInterfaceT <Memory> g_memory = { OLF_IMPORT_INTERFACE,
	"Nirvana/g_memory", CORBA::Internal::RepIdOf <Memory>::id,
	NIRVANA_STATIC_BRIDGE (Memory, Core::Memory) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_memory, "Nirvana/g_memory", Nirvana::Memory, Nirvana::Core::Memory)
