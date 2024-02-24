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
#include "pch.h"
#include "Synchronized.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

Synchronized::Synchronized (SyncContext& target, Heap* heap) :
	exec_domain_ (ExecDomain::current ()),
	call_context_ (&exec_domain_.sync_context ())
#ifndef NDEBUG
	, dbg_ed_mem_stack_size_ (exec_domain_.dbg_mem_context_stack_size_)
#endif
{
	// Target can not be legacy thread
	assert (target.sync_domain () || target.is_free_sync_context ());
	exec_domain_.schedule_call (target, heap);
}

Synchronized::Synchronized (SyncContext& target, Ref <MemContext>&& mem_context) :
	exec_domain_ (ExecDomain::current ()),
	call_context_ (&exec_domain_.sync_context ())
#ifndef NDEBUG
	, dbg_ed_mem_stack_size_ (exec_domain_.dbg_mem_context_stack_size_)
#endif
{
	// Target can not be legacy thread
	assert (target.sync_domain () || target.is_free_sync_context ());
	exec_domain_.schedule_call (target, std::move (mem_context));
}

Synchronized::~Synchronized ()
{
	if (call_context_) // If no exception and no suspend_and_return ()
		exec_domain_.schedule_return (*call_context_);
#ifndef NDEBUG
	assert (dbg_ed_mem_stack_size_ == exec_domain_.dbg_mem_context_stack_size_);
#endif
}

void Synchronized::suspend_and_return ()
{
	assert (call_context_);
	Ref <SyncContext> sync_context = std::move (call_context_);
	Ref <MemContext> mem_context = exec_domain_.mem_context_ptr ();
	exec_domain_.mem_context_pop ();
	try {
		exec_domain_.suspend (sync_context);
	} catch (...) {
		call_context_ = std::move (sync_context);
		exec_domain_.mem_context_push (std::move (mem_context));
		throw;
	}
}

void Synchronized::return_to_caller_context () noexcept
{
	if (call_context_) {
		Ref <SyncContext> context = std::move (call_context_);
		exec_domain_.schedule_return (*context, true);
	}
}

}
}
