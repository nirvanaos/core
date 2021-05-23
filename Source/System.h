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
#ifndef NIRVANA_CORE_SYSTEM_H_
#define NIRVANA_CORE_SYSTEM_H_

#include <CORBA/Server.h>
#include <generated/System_s.h>
#include "Binder.h"
#include "Chrono.h"
#include "Thread.inl"
#include "RuntimeSupportImpl.h"

namespace Nirvana {
namespace Core {

class System :
	public CORBA::servant_traits <Nirvana::System>::ServantStatic <System>
{
public:
	static RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		return get_runtime_support ().runtime_proxy_get (obj);
	}

	static void runtime_proxy_remove (const void* obj)
	{
		get_runtime_support ().runtime_proxy_remove (obj);
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

	Memory::_ref_type create_heap (uint16_t granularity)
	{
		throw_NO_IMPLEMENT ();
	}

private:
	static RuntimeSupport& get_runtime_support ()
	{
#if defined (NIRVANA_CORE_TEST) && defined (_DEBUG)
		static RuntimeSupportImpl <CoreAllocator> core_runtime_support;

		Thread* thread = Thread::current_ptr ();
		if (thread)
			return thread->runtime_support ();
		else
			return core_runtime_support;
#else
		return Thread::current ().runtime_support ();
#endif
	}
};

}
}

#endif
