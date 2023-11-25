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
#include <CORBA/Server.h>
#include <Nirvana/Legacy/Legacy_s.h>
#include <Legacy/Process.h>
#include <Legacy/Mutex.h>
#include "../TimerEvent.h"

namespace Nirvana {

using namespace Core;

namespace Legacy {
namespace Core {

class System :
	public CORBA::servant_traits <Legacy::System>::ServantStatic <System>
{
public:
	static Legacy::Thread::_ref_type create_thread (Runnable::_ptr_type)
	{
		throw_NO_IMPLEMENT (); // TODO:
	}

	static Legacy::Mutex::_ref_type create_mutex ()
	{
		return Mutex::create (ThreadBase::current ().process ());
	}

	static void sleep (TimeBase::TimeT period100ns)
	{
		if (!period100ns)
			ExecDomain::reschedule ();
		else {
			TimerEvent timer;
			timer.set (0, period100ns, 0);
			timer.wait ();
		}
	}
};

}

extern const ImportInterfaceT <System> NIRVANA_SELECTANY (g_system) = { OLF_IMPORT_INTERFACE,
	"Nirvana/Legacy/g_system", CORBA::Internal::RepIdOf <System>::id,
	NIRVANA_STATIC_BRIDGE (System, Core::System) };

}
}

#if !DISABLE_LEGACY_SUPPORT ()
NIRVANA_EXPORT (_exp_Nirvana_Legacy_g_system, "Nirvana/Legacy/g_system",
	Nirvana::Legacy::System, Nirvana::Legacy::Core::System)
#endif
