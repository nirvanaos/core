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
#ifndef NIRVANA_CORE_MEMCONTEXTCORE_H_
#define NIRVANA_CORE_MEMCONTEXTCORE_H_
#pragma once

#include "MemContext.h"
#include "TLS.h"
#include "StaticallyAllocated.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE MemContextCore :
	public MemContext
{
public:
	/// Creates MemContext object.
	/// 
	/// \returns MemContext reference.
	static Ref <MemContext> create ()
	{
		return Ref <MemContext>::create <ImplDynamic <MemContextCore> > ();
	}

	void* operator new (size_t cb);
	void operator delete (void* p, size_t cb);
	void* operator new (size_t cb, void* place);
	void operator delete (void*, void*);

protected:
	MemContextCore () NIRVANA_NOEXCEPT;
	~MemContextCore ();

	// MemContext methods

	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	virtual void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT;
	virtual void on_object_construct (MemContextObject& obj) NIRVANA_NOEXCEPT;
	virtual void on_object_destruct (MemContextObject& obj) NIRVANA_NOEXCEPT;
	virtual TLS& get_TLS () NIRVANA_NOEXCEPT;

private:
	TLS TLS_;
};

/// Shared memory context.
/// Used for allocation of the core objects to keep g_core_heap
/// small and fast.
/// 
/// NOTE: TLS object is not thread-safe. But TLS may be called only from the
///       startup execution domain. Do not share g_shared_mem_context with
///       other execution domains!
extern StaticallyAllocated <ImplStatic <MemContextCore> > g_shared_mem_context;

inline
bool MemContext::initialize () NIRVANA_NOEXCEPT
{
	if (!Port::Memory::initialize ())
		return false;
	g_core_heap.construct ();
	g_shared_mem_context.construct ();
	return true;
}

inline
void MemContext::terminate () NIRVANA_NOEXCEPT
{
	g_shared_mem_context.destruct ();
	g_core_heap.destruct ();
	Port::Memory::terminate ();
}

inline
void* MemContextCore::operator new (size_t cb)
{
	return g_shared_mem_context->heap ().allocate (nullptr, cb, 0);
}

inline
void MemContextCore::operator delete (void* p, size_t cb)
{
	g_shared_mem_context->heap ().release (p, cb);
}

inline
void* MemContextCore::operator new (size_t cb, void* place)
{
	return place;
}

inline
void MemContextCore::operator delete (void*, void*)
{}

}
}

#endif
