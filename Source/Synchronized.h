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

namespace Nirvana {
namespace Core {

class SyncDomain;

class Synchronized
{
public:
	Synchronized (SyncContext& target, MemContext* mem_context);
	~Synchronized ();

	/// \returns The caller synchronization context.
	SyncContext& call_context () const
	{
		return *call_context_;
	}

	/// Returns current execution domain.
	/// This method is quicker than ExecDomain::current ();
	ExecDomain& exec_domain () NIRVANA_NOEXCEPT
	{
		return exec_domain_;
	}

	/// Suspend execution.
	/// On resume, return to call context.
	void suspend_and_return ();

	/// Return to caller synchronization context.
	/// Skip rescehdule if possible.
	///  
	void return_to_caller_context ();

	/// Called on exception inside synchronization frame.
	NIRVANA_NORETURN void on_exception ();

private:
	Ref <SyncContext> call_context_;
	ExecDomain& exec_domain_;
};

}
}

#define SYNC_BEGIN(target, mem) { ::Nirvana::Core::Synchronized _sync_frame (target, mem); try {
#define SYNC_END() } catch (...) { _sync_frame.on_exception (); }}

#endif
