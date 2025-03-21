/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "ConditionalCreator.h"
#include "AtomicCounter.h"
#include <Port/config.h>

namespace Nirvana {
namespace Core {

class HeapDynamic :
	public Heap,
	public CoreObject,
	public ConditionalObjectBase <HEAP_POOLING>
{
	friend class ObjectCreator <Ref <HeapDynamic> >;
	using Creator = ConditionalCreator <Ref <HeapDynamic>, HEAP_POOLING>;
	friend class CORBA::servant_reference <HeapDynamic>;

public:
	static void initialize ()
	{
		Creator::initialize (HEAP_POOL_MIN);
	}

	static void terminate () noexcept
	{
		Creator::terminate ();
	}

	static Ref <Heap> create ()
	{
		return Creator::create ();
	}

private:
	HeapDynamic () :
		ref_cnt_ (1)
	{}

	void _add_ref () noexcept override
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept override
	{
		if (0 == ref_cnt_.decrement_seq ()) {
			cleanup (!HEAP_POOLING);
			Creator::release (this);
		}
	}

private:
	AtomicCounter <false> ref_cnt_;
};

}
}

#endif
