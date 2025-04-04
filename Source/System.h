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
#include "DeadlinePolicy.h"
#include "ExecDomain.h"
#include "SystemInfo.h"
#include "NameService/NameService.h"
#include "TLS.h"
#include "Binder.h"
#include "append_path.h"
#include "CurrentDir.h"
#include "EventUser.h"

namespace Nirvana {

class Static_the_system :
	public CORBA::servant_traits <System>::ServantStatic <Static_the_system>
{
public:
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

	static ContextType context_type ()
	{
		return (ContextType)Core::SyncContext::current ().sync_context_type ();
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

	void append_path (CosNaming::Name& name, const IDL::String& path, bool absolute)
	{
		Core::append_path (name, path, absolute);
	}

	static CosNaming::Name get_current_dir ()
	{
		return Core::CurrentDir::current_dir ();
	}

	// This operation can cause context switch.
	// So we make private copies of the client strings in stak.
	static CORBA::Internal::Interface::_ref_type bind (IDL::String name, IDL::String iid)
	{
		return Core::Binder::bind_interface (name, iid).itf;
	}

	static size_t exec_domain_id ()
	{
		return Core::ExecDomain::current ().id ();
	}

	static Platforms get_supported_platforms ()
	{
		return Platforms (Core::SystemInfo::supported_platforms (),
			Core::SystemInfo::supported_platforms () + Core::SystemInfo::SUPPORTED_PLATFORM_CNT);
	}

	static Event::_ref_type create_event (bool manual_reset, bool initial_state)
	{
		return CORBA::make_reference <Core::EventUser> (manual_reset, initial_state);
	}
};

}

#endif
