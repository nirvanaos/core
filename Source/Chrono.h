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
#ifndef NIRVANA_CORE_CHRONO_H_
#define NIRVANA_CORE_CHRONO_H_

#include <Port/Chrono.h>

namespace Nirvana {
namespace Core {

/// Core implementation of the Nirvana::Chrono service
class Chrono :
	private Port::Chrono
{
public:
	static uint16_t epoch () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::epoch;
	}

	typedef uint64_t Duration;

	static Duration system_clock () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::system_clock ();
	}

	static Duration steady_clock () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::steady_clock ();
	}

	static Duration system_to_steady (uint16_t _epoch, Duration system) NIRVANA_NOEXCEPT
	{
		assert (_epoch == epoch ()); // TODO: Implement
		return system - (system_clock () - steady_clock ());
	}

	static Duration steady_to_system (Duration steady) NIRVANA_NOEXCEPT
	{
		return steady + (system_clock () - steady_clock ());
	}

	static DeadlineTime make_deadline (Duration timeout) NIRVANA_NOEXCEPT
	{
		// TODO: If steady_clock_resoluion () is too low (1 sec?) we can implement advanced
		// algorithm to create diffirent deadlines inside one clock tick,
		// based on atomic counter.
		DeadlineTime dt = steady_clock ();
		assert (std::numeric_limits <DeadlineTime>::max () - timeout > dt);
		return dt + timeout;
	}
};

}
}

#endif
