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
#pragma once

#include <Port/Chrono.h>
#include <Nirvana/rescale.h>

namespace Nirvana {
namespace Core {

/// Core time service.
class Chrono : public Port::Chrono
{
public:
	static DeadlineTime deadline_from_priority (int16_t priority)
	{
		assert (priority >= 0);
		return deadline_clock () + deadline_clock_frequency () * ((int32_t)std::numeric_limits <int16_t>::max () - (int32_t)priority + 1) / TimeBase::MILLISECOND;
	}

	static int16_t deadline_to_priority (DeadlineTime deadline)
	{
		int64_t rem = deadline - deadline_clock ();
		if (rem < 0)
			rem = 0;
		int64_t ms = rescale64 (rem, TimeBase::MILLISECOND, deadline_clock_frequency () - 1, deadline_clock_frequency ());
		if (ms > std::numeric_limits <int16_t>::max ())
			ms = std::numeric_limits <int16_t>::max ();
		return std::numeric_limits <int16_t>::max () - (int16_t)ms;
	}
};

}
}

#endif
