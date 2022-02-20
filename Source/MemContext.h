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

namespace Nirvana {
namespace Core {

class MemContextObject;

/// Memory context.
/// Contains heap and some other stuff.
/// MemContext implementation is thread-safe.
/// A number of domains can share one MemContext object.
/// The MemContext implements heap only. Other virtual methods are stubbed.
///  only an is intended for use in Core only.
/// 
class NIRVANA_NOVTABLE MemContext
{
	DECLARE_CORE_INTERFACE

	MemContext (const MemContext&) = delete;
	MemContext& operator = (const MemContext&) = delete;

public:
	void* operator new (size_t cb);
	void operator delete (void* p, size_t cb);

	void* operator new (size_t cb, void* place)
	{
		return place;
	}

	void operator delete (void*, void*)
	{}

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

	/// Does nothing.
	/// Overridden in MemContextEx.
	virtual void on_object_construct (MemContextObject& obj);

	/// Does nothing.
	/// Overridden in MemContextEx.
	virtual void on_object_destruct (MemContextObject& obj);

	/// Global class initialization.
	static void initialize ();

	/// Global class temination.
	static void terminate () NIRVANA_NOEXCEPT;

protected:
	MemContext ();
	~MemContext ();

	void clear () NIRVANA_NOEXCEPT
	{
		heap_.cleanup ();
		// TODO: Detect and log memory leaks
	}

protected:
	HeapUser heap_;
};

/// Shared memory context.
/// Used for allocation of the core objects to keep g_core_heap
/// small and fast.
extern StaticallyAllocated <ImplStatic <MemContext>> g_shared_mem_context;

inline void MemContext::initialize ()
{
	Port::Memory::initialize ();
	g_core_heap.construct ();
	g_shared_mem_context.construct ();
}

inline void MemContext::terminate () NIRVANA_NOEXCEPT
{
	g_shared_mem_context.destruct ();
	g_core_heap.destruct ();
	Port::Memory::terminate ();
}

inline
void* MemContext::operator new (size_t cb)
{
	return g_shared_mem_context->heap ().allocate (nullptr, cb, 0);
}

inline
void MemContext::operator delete (void* p, size_t cb)
{
	g_shared_mem_context->heap ().release (p, cb);
}

}
}

#endif
