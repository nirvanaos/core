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
#include "Heap.h"
#include <Nirvana/HeapFactory_s.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

class HeapDynamic :
	public HeapUser,
	public CORBA::Nirvana::Servant <HeapDynamic, Memory>,
	public CORBA::Nirvana::LifeCycleRefCntImpl <HeapDynamic>
{
public:
	HeapDynamic (ULong allocation_unit) :
		HeapUser (allocation_unit)
	{}
};

class HeapFactoryImpl :
	public ::CORBA::Nirvana::ServantStatic <HeapFactoryImpl, HeapFactory>
{
public:
	static Memory_ptr create ()
	{
		return (new HeapDynamic (HEAP_UNIT_DEFAULT))->_get_ptr ();
	}
	static Memory_ptr create_with_granularity (ULong granularity)
	{
		return (new HeapDynamic (granularity))->_get_ptr ();
	}
};



}
}
