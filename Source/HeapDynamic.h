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
#ifndef NIRVANA_CORE_HEAPDYNAMIC_H_
#define NIRVANA_CORE_HEAPDYNAMIC_H_
#pragma once

#include "Heap.h"
#include <CORBA/Server.h>
#include "IDL/Memory_s.h"
#include "LifeCyclePseudo.h"
#include "MemContextObject.h"

namespace Nirvana {
namespace Core {

class HeapDynamic :
	public HeapUser,
	public CORBA::servant_traits <Nirvana::Memory>::Servant <HeapDynamic>,
	public LifeCyclePseudo <HeapDynamic>,
	public MemContextObject
{
public:
	static Nirvana::Memory::_ref_type create (uint16_t allocation_unit)
	{
		return CORBA::make_pseudo <HeapDynamic> (allocation_unit);
	}

	HeapDynamic (uint16_t allocation_unit) :
		HeapUser (allocation_unit)
	{}
};

}
}

#endif
