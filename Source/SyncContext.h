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
#ifndef NIRVANA_CORE_SYNCCONTEXT_H_
#define NIRVANA_CORE_SYNCCONTEXT_H_
#pragma once

#include "Thread.h"
#include "StaticallyAllocated.h"

namespace Nirvana {
namespace Core {

class Heap;
class ExecDomain;
class SyncDomain;
class MemContext;

/// Synchronization context pure interface.
/// Synchronization context may be associated with:
///   - Synchronization domain
///   - Free synchronization context for core stateless objects
///   - Free synchronization context for class library
///   - Legacy thread
class NIRVANA_NOVTABLE SyncContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	static SyncContext& current () NIRVANA_NOEXCEPT;

	/// Leave this context and enter to the other.
	/// 
	/// \param target Target synchronization context.
	virtual void schedule_call (SyncContext& target, MemContext* mem_context) = 0;

	/// Return execution domain to this context.
	/// 
	/// \param exec_domain Execution domain.
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT = 0;

	/// Returns SyncDomain associated with this context.
	/// Returns `nullptr` if there is not synchronization domain.
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;

	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data
	/// because all data are in stack or the execution domain heap and can not be changed
	/// by other threads during the synchronouse call.
	bool is_free_sync_context () const NIRVANA_NOEXCEPT
	{
		return const_cast <SyncContext&> (*this).stateless_memory () != nullptr;
	}

	/// Free sync context returns pointer to the stateless objects heap.
	/// Other sync contexts return `nullptr`.
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT;

protected:
	void check_schedule_error (ExecDomain& ed);
};

/// Free (not synchronized) sync context.
class NIRVANA_NOVTABLE SyncContextFree :
	public SyncContext
{
public:
	virtual void schedule_call (SyncContext& target, MemContext* mem_context);
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT = 0;
};

/// Core free sync context.
class SyncContextCore :
	public SyncContextFree
{
public:
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT;
};

extern StaticallyAllocated <ImplStatic <SyncContextCore>> g_core_free_sync_context;

}
}

#endif
