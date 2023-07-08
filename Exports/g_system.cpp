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
#include <Nirvana/System_s.h>
#include <Binder.h>
#include <Chrono.h>
#include <ExecDomain.h>
#include <HeapDynamic.h>
#include <Port/SystemInfo.h>
#include <Port/Debugger.h>
#include <Signals.h>
#include <Nirvana/Formatter.h>
#include <unrecoverable_error.h>
#include <NameService/FileSystem.h>

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

	static CORBA::Object::_ref_type bind (const IDL::String& name)
	{
		return Binder::bind (name);
	}

	static CORBA::Internal::Interface::_ref_type bind_interface (const IDL::String& name, const IDL::String& iid)
	{
		return Binder::bind_interface (name, iid);
	}

	static TimeBase::UtcT _s_system_clock (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::system_clock ();
	}

	static TimeBase::UtcT _s_UTC (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::UTC ();
	}

	static SteadyTime _s_steady_clock (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::steady_clock ();
	}

	static DeadlineTime _s_deadline_clock (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Chrono::deadline_clock ();
	}

	static CORBA::Internal::Type <DeadlineTime>::ConstRef _s_get_deadline_clock_frequency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return CORBA::Internal::Type <DeadlineTime>::ret (Chrono::deadline_clock_frequency ());
	}

	static DeadlineTime deadline_from_UTC (const TimeBase::UtcT& utc)
	{
		return Chrono::deadline_from_UTC (utc);
	}

	static TimeBase::UtcT deadline_to_UTC (const DeadlineTime& deadline)
	{
		return Chrono::deadline_to_UTC (deadline);
	}

	static DeadlineTime make_deadline (const TimeBase::TimeT& timeout)
	{
		return Chrono::make_deadline (timeout);
	}

	static const DeadlineTime& deadline ()
	{
		return ExecDomain::current ().deadline ();
	}

	static const DeadlinePolicy& deadline_policy_async ()
	{
		return ExecDomain::current ().deadline_policy_async ();
	}

	static void deadline_policy_async (const DeadlinePolicy& dp)
	{
		ExecDomain::current ().deadline_policy_async (dp);
	}

	static const DeadlinePolicy& deadline_policy_oneway ()
	{
		return ExecDomain::current ().deadline_policy_oneway ();
	}

	static void deadline_policy_oneway (const DeadlinePolicy& dp)
	{
		ExecDomain::current ().deadline_policy_oneway (dp);
	}

	static int* error_number ()
	{
		return &ExecDomain::current ().runtime_global_.error_number;
	}

	static Nirvana::Memory::_ref_type create_heap (uint16_t granularity)
	{
		return HeapDynamic::create (granularity);
	}

	static void raise (int signal)
	{
		Thread* th = Thread::current_ptr ();
		if (th) {
			ExecDomain* ed = th->exec_domain ();
			if (ed) {
				if (Signals::is_supported (signal))
					ed->raise (signal);
				else
					throw_BAD_PARAM ();
			}
			return;
		}
		unrecoverable_error (signal);
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

	static size_t _s_get_hardware_concurrency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	static bool is_legacy_mode ()
	{
		return SyncContext::current ().is_legacy_mode ();
	}

	static void debug_event (DebugEvent evt, const IDL::String& msg)
	{
		static const char* const ev_prefix [3] = {
			"INFO: ",
			"WARNING: ",
			"ERROR: "
		};
		// Use shared string to avoid possible problems with the memory context.
		SharedString s = ev_prefix [(unsigned)evt];
		s.append (msg.c_str (), msg.size ());
		s += '\n';
		Port::Debugger::output_debug_string (s.c_str ());
		if (DebugEvent::DEBUG_ERROR == evt) {
			if (!Port::Debugger::debug_break ())
				raise (SIGABRT);
		}
	}

	static void assertion_failed (const IDL::String& expr, const IDL::String& file_name, int32_t line_number)
	{
		// Use shared string to avoid possible problems with the memory context.
		SharedString s;
		if (!file_name.empty ()) {
			s.assign (file_name.c_str (), file_name.size ());
			s += '(';
			append_format (s, "%i", line_number);
			s += "): ";
		}
		s += "Assertion failed: ";
		s.append (expr.c_str (), expr.size ());
		s += '\n';
		Port::Debugger::output_debug_string (s.c_str ());
		if (!Port::Debugger::debug_break ())
			raise (SIGABRT);
	}

	static bool yield ()
	{
		return ExecDomain::reschedule ();
	}

	static uint16_t TLS_alloc ()
	{
		return TLS::allocate ();
	}

	static void TLS_free (uint16_t idx)
	{
		TLS::release (idx);
	}

	static void TLS_set (uint16_t idx, void* ptr)
	{
		if (idx < TLS::CORE_TLS_COUNT)
			throw_BAD_PARAM ();
		TLS::current ().set (idx, ptr);
	}

	static void* TLS_get (uint16_t idx)
	{
		return TLS::current ().get (idx);
	}

	static void get_name_from_path (CosNaming::Name& name, const IDL::String& path)
	{
		FileSystem::get_name_from_path (name, path);
	}

	static CosNaming::Name get_current_dir_name ()
	{
		return MemContext::current ().get_current_dir_name ();
	}

	static void chdir (const IDL::String& path)
	{
		MemContext::current ().chdir (path);
	}
};

}

NIRVANA_SELECTANY
const ImportInterfaceT <System> g_system = { OLF_IMPORT_INTERFACE,
	"Nirvana/g_system", CORBA::Internal::RepIdOf <System>::id,
	NIRVANA_STATIC_BRIDGE (System, Core::System) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_system, "Nirvana/g_system", Nirvana::System, Nirvana::Core::System)
