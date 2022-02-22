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
#include <CORBA/Server.h>
#include <signal.h>
#include <IDL/System_s.h>
#include <Binder.h>
#include <Chrono.h>
#include <ExecDomain.h>
#include <HeapDynamic.h>
#include <Port/SystemInfo.h>
#include <Port/Debugger.h>
#include <Signals.h>

namespace Nirvana {
namespace Core {

class System :
	public CORBA::servant_traits <Nirvana::System>::ServantStatic <System>
{
public:
	static RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		if (!RUNTIME_SUPPORT_DISABLE)
			return MemContext::current ().runtime_proxy_get (obj);
		else
			return nullptr;
	}

	static void runtime_proxy_remove (const void* obj)
	{
		if (!RUNTIME_SUPPORT_DISABLE)
			MemContext::current ().runtime_proxy_remove (obj);
	}

	static CORBA::Object::_ref_type bind (const std::string& name)
	{
		return Binder::bind (name);
	}

	static CORBA::Internal::Interface::_ref_type bind_interface (const std::string& name, const std::string& iid)
	{
		return Binder::bind_interface (name, iid);
	}

	static uint16_t __get_epoch (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::epoch ();
	}

	static Duration __get_system_clock (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::system_clock ();
	}

	static Duration __get_steady_clock (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
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
		return HeapDynamic::create (granularity);
	}

	static void raise (int signal)
	{
		if (Signals::is_supported (signal))
			Thread::current ().exec_domain ()->raise (signal);
		else
			throw_BAD_PARAM ();
	}

	static void sigaction (int signal, const struct sigaction* act, struct sigaction* oldact)
	{
		throw_NO_IMPLEMENT ();
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

	static size_t __get_hardware_concurrency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	static bool is_legacy_mode ()
	{
		return Thread::current ().is_legacy ();
	}

	static void debug_event (DebugEvent evt, const std::string& msg)
	{
		static const char* ev_prefix [3] = {
			"INFO: ",
			"WARNING: ",
			"ERROR: "
		};
		std::string s = ev_prefix [(unsigned)evt];
		s += msg;
		s += '\n';
		Port::Debugger::output_debug_string (s.c_str ());
		if (DebugEvent::DEBUG_ERROR == evt)
			Port::Debugger::debug_break ();
	}

	static bool yield ()
	{
		return ExecDomain::yield ();
	}

	static uint16_t TLS_alloc ()
	{
		return TLS::allocate ();
	}

	static void TLS_free (uint16_t idx)
	{
		TLS::release (idx);
	}

	static void TLS_set (uint16_t idx, void* ptr, Deleter del)
	{
		if (idx < TLS::CORE_TLS_COUNT)
			throw_BAD_PARAM ();
		MemContext::current ().get_TLS ().set (idx, ptr, del);
	}

	static void* TLS_get (uint16_t idx)
	{
		return MemContext::current ().get_TLS ().get (idx);
	}
};

}

__declspec (selectany)
const ImportInterfaceT <System> g_system = { OLF_IMPORT_INTERFACE, "Nirvana/g_system", System::repository_id_, NIRVANA_STATIC_BRIDGE (System, Core::System) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_system, "Nirvana/g_system", Nirvana::System, Nirvana::Core::System)
