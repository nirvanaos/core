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
#include "ORB/POA.h"

namespace Nirvana {
namespace Core {

void initialize0 ()
{
	Heap::initialize ();
	g_core_free_sync_context.construct ();
	ExecDomain::initialize ();
	Scheduler::initialize ();
	CORBA::Internal::Core::g_root_POA.construct ();
}

void initialize ()
{
	Binder::initialize ();
}

void terminate ()
{
	Binder::terminate ();
	CORBA::Internal::Core::g_root_POA.destruct ();
}

void terminate0 () NIRVANA_NOEXCEPT
{
	Scheduler::terminate ();
	ExecDomain::terminate ();
	//g_core_free_sync_context.destruct ();
	Heap::terminate ();
}

}
}