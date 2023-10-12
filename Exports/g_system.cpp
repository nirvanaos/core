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

	static CORBA::Internal::Type <DeadlineTime>::ConstRef _s__get_deadline_clock_frequency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
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
		return MemContext::current ().deadline_policy_async ();
	}

	static void deadline_policy_async (const DeadlinePolicy& dp)
	{
		MemContext::current ().deadline_policy_async (dp);
	}

	static const DeadlinePolicy& deadline_policy_oneway ()
	{
		return MemContext::current ().deadline_policy_oneway ();
	}

	static void deadline_policy_oneway (const DeadlinePolicy& dp)
	{
		MemContext::current ().deadline_policy_oneway (dp);
	}

	static int* error_number ()
	{
		return RuntimeGlobal::current ().error_number ();
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

	static void srand (unsigned seed)
	{
		RuntimeGlobal::current ().srand (seed);
	}

	static int rand ()
	{
		return RuntimeGlobal::current ().rand ();
	}

	static size_t _s__get_hardware_concurrency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	static bool is_legacy_mode ()
	{
		return SyncContext::current ().is_legacy_mode ();
	}

	static void debug_event (DebugEvent evt, const IDL::String& msg, const IDL::String& file_name, int32_t line_number)
	{
		// Use shared string to avoid possible problems with the memory context.
		SharedString s;

		if (!file_name.empty ()) {
			s.assign (file_name.c_str (), file_name.size ());
			s += '(';
			append_format (s, "%i", line_number);
			s += "): ";
		}

		static const char* const ev_prefix [(size_t)DebugEvent::DEBUG_ERROR + 1] = {
			"INFO: ",
			"WARNING: ",
			"Assertion failed: ",
			"ERROR: "
		};

		s += ev_prefix [(unsigned)evt];
		s += msg;
		s += '\n';
		Port::Debugger::output_debug_string (evt, s.c_str ());
		if (evt >= DebugEvent::DEBUG_WARNING) {
			if (!Port::Debugger::debug_break () && evt >= DebugEvent::DEBUG_ASSERT)
				__debugbreak ();
			//raise (SIGABRT);
		}
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

	static void TLS_set (uint16_t idx, void* ptr, Deleter deleter)
	{
		TLS::set (idx, ptr, deleter);
	}

	static void* TLS_get (uint16_t idx)
	{
		return TLS::get (idx);
	}

	static void get_name_from_path (CosNaming::Name& name, const IDL::String& path)
	{
		FileSystem::get_name_from_path (name, path);
	}

	static CosNaming::Name get_current_dir_name ()
	{
		return MemContextUser::current ().get_current_dir_name ();
	}

	static void chdir (const IDL::String& path)
	{
		MemContextUser::current ().chdir (path);
	}

	static uint16_t fd_open (const IDL::String& path, uint_fast16_t flags, uint_fast16_t mode)
	{
		return (uint16_t)MemContextUser::current ().fd_open (path, flags, mode);
	}

	static void fd_close (unsigned fd)
	{
		MemContextUser::current ().fd_close (fd);
	}

	static size_t fd_read (unsigned fd, void* p, size_t size)
	{
		return MemContextUser::current ().fd_read (fd, p, size);
	}

	static void fd_write (unsigned fd, const void* p, size_t size)
	{
		MemContextUser::current ().fd_write (fd, p, size);
	}

	static FileSize fd_seek (unsigned fd, const FileOff& offset, uint_fast16_t whence)
	{
		return MemContextUser::current ().fd_seek (fd, offset, whence);
	}

	static int_fast16_t fcntl (unsigned fd, int_fast16_t cmd, uint_fast16_t arg)
	{
		return MemContextUser::current ().fcntl (fd, cmd, arg);
	}

};

}

NIRVANA_SELECTANY
const ImportInterfaceT <System> g_system = { OLF_IMPORT_INTERFACE,
	"Nirvana/g_system", CORBA::Internal::RepIdOf <System>::id,
	NIRVANA_STATIC_BRIDGE (System, Core::System) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_system, "Nirvana/g_system", Nirvana::System, Nirvana::Core::System)
