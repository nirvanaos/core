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

#include <Nirvana/Nirvana.h>
#include <Nirvana/System_s.h>
#include "Chrono.h"
#include "DeadlinePolicy.h"
#include "ExecDomain.h"
#include <Port/SystemInfo.h>
#include "NameService/NameService.h"
#include "TLS.h"

namespace Nirvana {

class Static_the_system :
	public CORBA::servant_traits <System>::ServantStatic <Static_the_system>
{
public:
	static TimeBase::UtcT _s_system_clock (CORBA::Internal::Bridge <System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::system_clock ();
	}

	static TimeBase::UtcT _s_UTC (CORBA::Internal::Bridge <System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::UTC ();
	}

	static SteadyTime _s_steady_clock (CORBA::Internal::Bridge <System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::steady_clock ();
	}

	static DeadlineTime _s_deadline_clock (CORBA::Internal::Bridge <System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::deadline_clock ();
	}

	static IDL::Type <DeadlineTime>::ConstRef _s__get_deadline_clock_frequency (CORBA::Internal::Bridge <System>* _b, CORBA::Internal::Interface* _env)
	{
		return IDL::Type <DeadlineTime>::ret (Core::Chrono::deadline_clock_frequency ());
	}

	static DeadlineTime deadline_from_UTC (const TimeBase::UtcT& utc)
	{
		return Core::Chrono::deadline_from_UTC (utc);
	}

	static TimeBase::UtcT deadline_to_UTC (const DeadlineTime& deadline)
	{
		return Core::Chrono::deadline_to_UTC (deadline);
	}

	static DeadlineTime make_deadline (const TimeBase::TimeT& timeout)
	{
		return Core::Chrono::make_deadline (timeout);
	}

	static const DeadlineTime& deadline ()
	{
		return Core::ExecDomain::current ().deadline ();
	}

	static const DeadlinePolicy& deadline_policy_async ()
	{
		return Core::DeadlinePolicy::policy_async ();
	}

	static void deadline_policy_async (const DeadlinePolicy& dp)
	{
		Core::DeadlinePolicy::policy_async (dp);
	}

	static const DeadlinePolicy& deadline_policy_oneway ()
	{
		return Core::DeadlinePolicy::policy_oneway ();
	}

	static void deadline_policy_oneway (const DeadlinePolicy& dp)
	{
		Core::DeadlinePolicy::policy_oneway (dp);
	}

	static bool yield ()
	{
		return Core::ExecDomain::reschedule ();
	}

	static uint32_t _s__get_hardware_concurrency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Port::SystemInfo::hardware_concurrency ();
	}

	static ContextType context_type ()
	{
		return (ContextType)Core::SyncContext::current ().sync_context_type ();
	}

	static uint16_t TLS_alloc (Deleter deleter)
	{
		return Core::TLS::allocate (deleter);
	}

	static void TLS_free (uint16_t idx)
	{
		Core::TLS::release (idx);
	}

	static void TLS_set (uint16_t idx, void* ptr)
	{
		Core::TLS::set (idx, ptr);
	}

	static void* TLS_get (uint16_t idx)
	{
		return Core::TLS::get (idx);
	}

	static IDL::String to_string (const CosNaming::Name& name)
	{
		CosNaming::Core::NamingContextRoot::check_name (name);
		return CosNaming::Core::NameService::to_string_unchecked (name);
	}

	static CosNaming::Name to_name (const IDL::String& sn)
	{
		return CosNaming::Core::NameService::to_name (sn);
	}

	static size_t exec_domain_id ()
	{
		return Core::ExecDomain::current ().id ();
	}
};

}

#endif
