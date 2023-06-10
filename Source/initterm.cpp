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
#include "initterm.h"
#include "Binder.h"
#include "Scheduler.h"
#include "ExecDomain.h"
#include "Timer.h"
#include "TLS.h"
#include "ORB/ORB_initterm.h"
#include "ORB/Services.h"
#include "ORB/LocalAddress.h"
#include "ORB/POA_Root.h"
#include <Port/PostOffice.h>
#include "ProtDomain.h"

namespace Nirvana {
namespace Core {

bool initialize0 () noexcept
{
	if (!Heap::initialize ())
		return false;
	TLS::initialize ();
	g_core_free_sync_context.construct ();
	ExecDomain::initialize ();
	Scheduler::initialize ();
	return true;
}

void initialize ()
{
	Timer::initialize ();

	// CORBA::Core::Services must be initialized before Binder
	CORBA::Core::initialize ();

	Binder::initialize ();

	ProtDomain::initialize ();
	CORBA::Core::Services::bind (CORBA::Core::Services::ProtDomain);

	// Start receiving messages from other domains
	Port::PostOffice::initialize (CORBA::Core::LocalAddress::singleton ().host (), CORBA::Core::LocalAddress::singleton ().port ());
}

void shutdown ()
{
	// Disable all timers
	Timer::terminate ();

	// Block incoming requests and complete currently executed ones.
	PortableServer::Core::POA_Root::shutdown ();

	// Terminate services to release all proxies
	CORBA::Core::Services::terminate ();

	// Clear remote references, if any
	Binder::clear_remote_references ();
}

void terminate ()
{
	// Stop receiving messages. But we can still send messages.
	Port::PostOffice::terminate ();

	ProtDomain::terminate ();
	Binder::terminate ();
	CORBA::Core::terminate ();
}

void terminate0 () noexcept
{
	Scheduler::terminate ();
	ExecDomain::terminate ();
#ifdef _DEBUG
	g_core_free_sync_context.destruct ();
	Heap::terminate ();
#endif
}

}
}
