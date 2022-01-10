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
#ifndef NIRVANA_CORE_SYSTEM_H_
#define NIRVANA_CORE_SYSTEM_H_

#include <CORBA/Server.h>
#include <System_s.h>
#include "Binder.h"
#include "Chrono.h"
#include "ExecDomain.h"
#include "RuntimeSupportImpl.h"
#include "HeapDynamic.h"

namespace Nirvana {
namespace Core {

class System :
	public CORBA::servant_traits <Nirvana::System>::ServantStatic <System>
{
public:
	static RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		RuntimeSupport* rs = get_runtime_support ();
		if (rs)
			return rs->runtime_proxy_get (obj);
		return nullptr;
	}

	static void runtime_proxy_remove (const void* obj)
	{
		RuntimeSupport* rs = get_runtime_support ();
		if (rs)
			rs->runtime_proxy_remove (obj);
	}

	static CORBA::Object::_ref_type bind (const std::string& name)
	{
		return Binder::bind (name);
	}

	static CORBA::Internal::Interface::_ref_type bind_interface (const std::string& name, const std::string& iid)
	{
		return Binder::bind_interface (name, iid);
	}

	static uint16_t epoch ()
	{
		return Chrono::epoch ();
	}

	static Duration system_clock ()
	{
		return Chrono::system_clock ();
	}

	static Duration steady_clock ()
	{
		return Chrono::steady_clock ();
	}

	static Duration system_to_steady (const uint16_t& epoch, const Duration& clock)
	{
		return Chrono::system_to_steady (epoch, clock);
	}

	static Duration steady_to_system (const Duration& steady)
	{
		return Chrono::steady_to_system (steady);
	}

	static Duration deadline ()
	{
		return Thread::current ().exec_domain ()->deadline ();
	}

	static int* error_number ()
	{
		return &Thread::current ().exec_domain ()->runtime_global_.error_number;
	}

	static Nirvana::Memory::_ref_type create_heap (uint16_t granularity)
	{
		return CORBA::make_pseudo <HeapDynamic> (granularity);
	}

	static void abort ()
	{
		Thread::current ().exec_domain ()->abort ();
	}

	static void srand (uint32_t seed)
	{
		Thread::current ().exec_domain ()->runtime_global_.rand_state = seed;
	}

	static short rand () // RAND_MAX assumed to be 32767
	{
		uint32_t& next = Thread::current ().exec_domain ()->runtime_global_.rand_state;
		next = next * 1103515245 + 12345;
		return (unsigned int)(next >> 16) % 32768;
	}

private:
	static RuntimeSupport* get_runtime_support ()
	{
		Thread* th = Thread::current_ptr ();
		if (th) {
			ExecDomain* ed = th->exec_domain ();
			if (ed)
				return &ed->sync_context ().runtime_support ();
		}
		return nullptr;
	}
};

}
}

#endif
