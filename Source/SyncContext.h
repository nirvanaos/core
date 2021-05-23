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

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Heap;
class ExecDomain;
class SyncDomain;

/// Core synchronization context.
class NIRVANA_NOVTABLE SyncContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	static SyncContext& current () NIRVANA_NOEXCEPT;

	/// Returns free synchronization context.
	static SyncContext& free_sync_context () NIRVANA_NOEXCEPT;

	static SyncDomain* SUSPEND ()
	{
		return (SyncDomain*)(~(uintptr_t)0);
	}

	/// Leave this context and enter to the synchronization domain.
	virtual void schedule_call (SyncDomain* sync_domain) = 0;

	/// Return execution domain to this context.
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT = 0;

	/// Returns `nullptr` if it is free sync context.
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT = 0;

	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data
	/// because all data are in stack or the execution domain heap and can not be changed
	/// by other threads during the synchronouse call.
	bool is_free_sync_context () const NIRVANA_NOEXCEPT
	{
		return this == &free_sync_context ();
	}

	/// Returns Heap reference.
	virtual Heap& memory () NIRVANA_NOEXCEPT = 0;

	/// Returns RuntimeSupport reference.
	virtual RuntimeSupport& runtime_support () NIRVANA_NOEXCEPT = 0;

protected:
	void check_schedule_error (ExecDomain& ed);
};

}
}

#endif
