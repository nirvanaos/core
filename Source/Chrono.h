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

#include <CORBA/Server.h>
#include <generated/Chrono_s.h>
#include <Port/Chrono.h>

namespace Nirvana {
namespace Core {

class Chrono :
	public CORBA::servant_traits <Nirvana::Chrono>::ServantStatic <Chrono>,
	private Port::Chrono
{
public:
	static uint16_t epoch () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::epoch;
	}

	static uint64_t system_clock () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::system_clock ();
	}

	static uint64_t steady_clock () NIRVANA_NOEXCEPT
	{
		return Port::Chrono::steady_clock ();
	}

	static uint64_t system_to_steady (uint16_t _epoch, uint64_t system) NIRVANA_NOEXCEPT
	{
		assert (_epoch == epoch ()); // TODO: Implement
		return system - (system_clock () - steady_clock ());
	}

	static uint64_t steady_to_system (uint64_t steady) NIRVANA_NOEXCEPT
	{
		return steady + (system_clock () - steady_clock ());
	}

	static uint64_t create_deadline (uint64_t timeout) NIRVANA_NOEXCEPT
	{
		// TODO: We can use advanced algorithm for generation different deadlines inside one steady clock tick.
		// See Port::Chrono::steady_clock_resoluion ().
		return steady_clock () + timeout;
	}
};

}
}

#endif
