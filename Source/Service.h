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
#ifndef NIRVANA_CORE_SERVICE_H_
#define NIRVANA_CORE_SERVICE_H_
#pragma once

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

class ServiceMemorySeparate
{
public:
	MemContext& memory () NIRVANA_NOEXCEPT
	{
		return memory_;
	}

private:
	ImplStatic <MemContextCore> memory_;
};

class ServiceMemoryShared
{
public:
	static MemContext& memory () NIRVANA_NOEXCEPT
	{
		return g_shared_mem_context;
	}
};

using ServiceMemory = std::conditional_t <(sizeof (void*) > 4), ServiceMemorySeparate, ServiceMemoryShared>;

class Service : protected ServiceMemory
{
	DECLARE_CORE_INTERFACE

protected:
	Service () :
		sync_domain_ (std::ref (memory ()))
	{}

	SyncDomain& sync_domain () NIRVANA_NOEXCEPT
	{
		return sync_domain_;
	}

private:
	ImplStatic <SyncDomainCore> sync_domain_;
};

}
}

#endif
