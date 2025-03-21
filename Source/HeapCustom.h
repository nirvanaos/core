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
#ifndef NIRVANA_CORE_HEAPCUSTOM_H_
#define NIRVANA_CORE_HEAPCUSTOM_H_
#pragma once

#include <CORBA/Server.h>
#include "Heap.h"
#include "UserObject.h"
#include <Nirvana/Memory_s.h>

namespace Nirvana {
namespace Core {

class HeapCustom :
	public Heap,
	public CORBA::servant_traits <Nirvana::Memory>::Servant <HeapCustom>,
	public CORBA::Internal::LifeCycleRefCnt <HeapCustom>,
	public CORBA::Internal::RefCountBase <HeapCustom>,
	public UserObject
{
	friend class CORBA::Internal::LifeCycleRefCnt <HeapCustom>;

public:
	using UserObject::operator new;
	using UserObject::operator delete;

	static Nirvana::Memory::_ref_type create (size_t allocation_unit);

	static Nirvana::Memory::_ref_type create_heap (size_t granularity)
	{
		return create (granularity);
	}

	HeapCustom (size_t allocation_unit) :
		Heap (allocation_unit)
	{}

private:
	virtual void _add_ref () noexcept override;
	virtual void _remove_ref () noexcept override;

};

}
}

#endif
