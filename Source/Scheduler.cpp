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
#include "Scheduler.h"
#include "ExecDomain.h"
#include "initterm.h"
#include "ORB/POA_Root.h"
#include <Port/PostOffice.h>
#include "unrecoverable_error.h"

//#define DEBUG_SHUTDOWN

namespace Nirvana {
namespace Core {

void Scheduler::Shutdown::run ()
{
	do_shutdown ();
}

void Scheduler::Terminator::run ()
{
	// Terminate
#ifdef DEBUG_SHUTDOWN
	g_system->debug_event (System::DebugEvent::DEBUG_INFO, "Shutdown 1");
#endif
	Core::terminate ();
}

StaticallyAllocated <Scheduler::GlobalData> Scheduler::global_;

void Scheduler::do_shutdown ()
{
	// Block incoming requests and complete currently executed ones.
	PortableServer::Core::POA_Root::shutdown ();
	// Terminate services to release all proxies
	CORBA::Core::Services::terminate ();
	// Stop receiving messages
	Port::PostOffice::terminate ();
	// If no activity - toggle it.
	if (!global_->activity_cnt) {
		global_->activity_cnt.increment ();
		activity_end ();
	}
}

void Scheduler::shutdown ()
{
	State state = State::RUNNING;
	if (global_->state.compare_exchange_strong (state, State::SHUTDOWN_STARTED)) {
#ifdef DEBUG_SHUTDOWN
		g_system->debug_event (System::DebugEvent::DEBUG_INFO, "Shutdown 0");
#endif
		if (Thread::current_ptr ()) // Called from worker thread
			do_shutdown ();
		else
			ExecDomain::async_call <Shutdown> (INFINITE_DEADLINE, g_core_free_sync_context, nullptr);
	}
}

void Scheduler::activity_end () NIRVANA_NOEXCEPT
{
	if (!global_->activity_cnt.decrement_seq ()) {
		switch (global_->state) {
			case State::SHUTDOWN_STARTED: {
				State state = State::SHUTDOWN_STARTED;
				if (global_->state.compare_exchange_strong (state, State::TERMINATE)) {
					int* p = nullptr;
					*p = 0;
					try {
						ExecDomain::async_call <Terminator> (INFINITE_DEADLINE, g_core_free_sync_context, nullptr);
					} catch (...) {
						// Fallback
						activity_begin ();
						activity_end ();
					}
				}
			} break;

			case State::TERMINATE: {
				State state = State::TERMINATE;
				if (global_->state.compare_exchange_strong (state, State::SHUTDOWN_FINISH)) {
					Port::Scheduler::shutdown ();
				}
			} break;
		}
	}
}

}
}
