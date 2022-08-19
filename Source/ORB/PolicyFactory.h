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
#ifndef NIRVANA_ORB_CORE_POLICYFACTORY_H_
#define NIRVANA_ORB_CORE_POLICYFACTORY_H_
#pragma once

#include <CORBA/CORBA.h>
#include <algorithm>

namespace CORBA {
namespace Core {

class PolicyFactory
{
	static const size_t SUPPORTED_CNT = 7;

public:
	static Policy::_ref_type create_policy (PolicyType type, const Any& val)
	{
		auto* pc = std::lower_bound (creators, std::end (creators), type, comp);
		if (pc != std::end (creators))
			return (pc->creator) (val);

		throw PolicyError (UNSUPPORTED_POLICY);
	}

private:
	struct Creator
	{
		PolicyType type;
		Policy::_ref_type (*creator) (const Any&);
	};

	static bool comp (const Creator& l, PolicyType r) NIRVANA_NOEXCEPT
	{
		return l.type < r;
	}

	static const Creator creators [SUPPORTED_CNT];
};

}
}

#endif
