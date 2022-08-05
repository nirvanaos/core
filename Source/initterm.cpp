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
#include "TLS.h"
#include "ORB/CORBA_initterm.h"

namespace Nirvana {
namespace Core {

void initialize0 ()
{
	MemContext::initialize ();
	TLS::initialize ();
	g_core_free_sync_context.construct ();
	ExecDomain::initialize ();
	Scheduler::initialize ();
}

void initialize ()
{
	CORBA::Core::initialize ();
	Binder::initialize ();
}

void terminate ()
{
	Binder::terminate ();
	CORBA::Core::terminate ();
}

void terminate0 () NIRVANA_NOEXCEPT
{
	Scheduler::terminate ();
	ExecDomain::terminate ();
#ifdef _DEBUG
	g_core_free_sync_context.destruct ();
	MemContext::terminate ();
#endif
}

}
}
