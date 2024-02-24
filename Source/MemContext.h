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
#include <Nirvana/System.h>

namespace Nirvana {
namespace Core {

class MemContextUser;

/// Memory context.
/// Contains heap and some other stuff.
///
/// Unlike the heap, memory context is not thread-safe and and must be used by exactly one
/// execution context at the given time.
/// A number of the memory contexts can share the one heap.
/// MemContext can not be static.
class NIRVANA_NOVTABLE MemContext
{
	template <class>
	friend class CORBA::servant_reference;

public:
	/// \returns Current memory context.
	///          If there is no current memory context, a new user memory context will be created.
	/// \throws CORBA::NO_MEMORY.
	static MemContext& current ();

	/// Check if specific memory context is current.
	/// 
	/// \param context A memory context pointer.
	/// \returns `true` if \p context is current memory context.
	static bool is_current (const MemContext* context) noexcept;

	/// \returns Heap.
	Heap& heap () noexcept
	{
		return *heap_;
	}

	/// \returns If this is an user memory context, returns MemContextUser pointer.
	///          For the core memory context, returns `false.
	MemContextUser* user_context () noexcept;

	const DeadlineTime& deadline_policy_async () const noexcept
	{
		return deadline_policy_async_;
	}

	void deadline_policy_async (const DeadlineTime& dp) noexcept
	{
		deadline_policy_async_ = dp;
	}

	const DeadlineTime& deadline_policy_oneway () const noexcept
	{
		return deadline_policy_oneway_;
	}

	void deadline_policy_oneway (const DeadlineTime& dp) noexcept
	{
		deadline_policy_oneway_ = dp;
	}

	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) = 0;

	/// Remove runtime proxy for object \p obj.
	/// 
	/// \param obj Pointer used as a key.
	virtual void runtime_proxy_remove (const void* obj) noexcept = 0;

protected:
	MemContext (Ref <Heap>&& heap, bool user) noexcept;

	template <class Impl>
	static Ref <MemContext> create (Ref <Heap>&& heap)
	{
		size_t cb = sizeof (Impl);
		return CreateRef (new (heap->allocate (nullptr, cb, 0)) Impl (std::move (heap)));
	}

	template <class Impl>
	static Ref <MemContext> create ()
	{
		return create <Impl> (create_heap ());
	}

	static Ref <Heap> create_heap ();

	virtual ~MemContext ();

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	void operator delete (void* p, size_t cb)
	{
		MemContext::current ().heap ().release (p, cb);
	}

private:
	class CreateRef : public Ref <MemContext>
	{
	public:
		CreateRef (MemContext* p) :
			Ref <MemContext> (p, false)
		{}
	};

private:
	Ref <Heap> heap_;
	DeadlineTime deadline_policy_async_;
	DeadlineTime deadline_policy_oneway_;
	RefCounter ref_cnt_;
	const bool user_;
};

}
}

#endif
