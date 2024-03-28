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
#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_
#pragma once

#include "SyncContext.h"
#include "ORB/SystemExceptionHolder.h"

namespace Nirvana {
namespace Core {

class MemContext;

class Synchronized
{
public:
	Synchronized (SyncContext& target, Heap* heap);
	Synchronized (SyncContext& target, Ref <MemContext>&& mem_context);
	~Synchronized ();

	/// \returns The caller synchronization context.
	SyncContext& call_context () const
	{
		return *call_context_;
	}

	/// Returns current execution domain.
	/// This method is quicker than ExecDomain::current ();
	ExecDomain& exec_domain () noexcept
	{
		return exec_domain_;
	}

	/// Prepare to suspend execution.
	/// exec_domain ().suspend () must be called immediately after successfull call.
	/// On resume, esecution will return to the caller synchronization context.
	void prepare_suspend_and_return ();

	/// Return to the caller synchronization context.
	/// Skip reschedule if possible.
	///  
	void return_to_caller_context () noexcept;

	/// Called on exception inside synchronization frame.
	void on_exception (const CORBA::Exception& ex) noexcept
	{
		exception_.set_exception (ex);
		return_to_caller_context ();
	}

	void check () const
	{
		exception_.check ();
	}

private:
	ExecDomain& exec_domain_;
	Ref <SyncContext> call_context_;
	CORBA::Core::SystemExceptionHolder exception_;
#ifndef NDEBUG
	size_t dbg_ed_mem_stack_size_;
#endif
};

}
}

#define SYNC_BEGIN(target, mem) { ::Nirvana::Core::Synchronized _sync_frame (target, mem); try {
#define SYNC_END() } catch (const CORBA::Exception& ex) { _sync_frame.on_exception (ex); } _sync_frame.check (); }

#endif
