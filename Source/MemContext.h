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
#ifndef NIRVANA_CORE_MEMCONTEXT_H_
#define NIRVANA_CORE_MEMCONTEXT_H_
#pragma once

#include "HeapUser.h"
#include "CoreInterface.h"
#include "CoreObject.h"

namespace Nirvana {
namespace Core {

/// Memory context.
/// Contains heap and some other stuff.
/// MemContext implementation is thread-safe.
/// A number of domains can share one MemContext object.
/// The MemContext implements heap only. Other virtual methods are stubbed.
///  only an is intended for use in Core only.
/// 
class NIRVANA_NOVTABLE MemContext :
	public CoreObject,
	public CoreInterface
{
	MemContext (const MemContext&) = delete;
	MemContext& operator = (const MemContext&) = delete;

public:
	/// \returns Current memory context.
	static MemContext& current ();

	static bool is_current (MemContext* context);

	/// Creates MemContext object.
	/// 
	/// \returns MemContext reference.
	static CoreRef <MemContext> create ()
	{
		return CoreRef <MemContext>::create <ImplDynamic <MemContext>> ();
	}

	/// \returns User heap.
	Heap& heap () NIRVANA_NOEXCEPT
	{
		return heap_;
	}

	/// Returns `nullptr`.
	/// Overridden in MemContextEx.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);

	/// Does nothing.
	/// Overridden in MemContextEx.
	virtual void runtime_proxy_remove (const void* obj);

	/// Throws CORBA::NO_IMPLEMENT.
	/// Overridden in MemContextEx.
	virtual Memory::_ref_type create_heap (uint16_t granularity);

protected:
	MemContext ();
	~MemContext ();

protected:
	HeapUser heap_;
};

/// To reduce memory consumtion, core syncronization domains for some
/// lightweight core objects share one memory context.
extern StaticallyAllocated <ImplStatic <MemContext>> g_shared_mem_context;

}
}

#endif
