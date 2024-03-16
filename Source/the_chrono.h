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
#ifndef NIRVANA_CORE_THE_CHRONO_H_
#define NIRVANA_CORE_THE_CHRONO_H_
#pragma once

#include "Chrono.h"
#include "DeadlinePolicy.h"
#include "TimerEvent.h"
#include <Nirvana/Chrono_s.h>

namespace Nirvana {

class Static_the_chrono :
	public IDL::traits <Chrono>::ServantStatic <Static_the_chrono>
{
public:
	static TimeBase::UtcT _s_system_clock (CORBA::Internal::Bridge <Chrono>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::system_clock ();
	}

	static TimeBase::UtcT _s_UTC (CORBA::Internal::Bridge <Chrono>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::UTC ();
	}

	static SteadyTime _s_steady_clock (CORBA::Internal::Bridge <Chrono>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::steady_clock ();
	}

	static DeadlineTime _s_deadline_clock (CORBA::Internal::Bridge <Chrono>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Chrono::deadline_clock ();
	}

	static IDL::Type <DeadlineTime>::ConstRef _s__get_deadline_clock_frequency (CORBA::Internal::Bridge <Chrono>* _b, CORBA::Internal::Interface* _env)
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

	static void sleep (TimeBase::TimeT period100ns)
	{
		if (!period100ns)
			Core::ExecDomain::reschedule ();
		else {
			Core::TimerEvent timer;
			timer.set (0, period100ns, 0);
			timer.wait ();
		}
	}

};

}

#endif
