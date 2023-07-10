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
	friend class CORBA::servant_reference <MemContext>;

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

protected:
	MemContext (bool user);

	MemContext (Heap& heap, bool user) noexcept :
		heap_ (&heap),
		user_ (user)
	{}

	virtual ~MemContext ();

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

protected:
	Ref <Heap> heap_;

private:
	RefCounter ref_cnt_;
	const bool user_;
};

}
}

#endif
