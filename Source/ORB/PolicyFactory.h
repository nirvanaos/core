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

/// COMPRESSION_MIN_RATIO_POLICY_ID = 67.
/// See ZIOP::CompressionMinRatioPolicy.
#define MAX_KNOWN_POLICY_ID 67

namespace CORBA {
namespace Core {

class PolicyFactory
{
public:
	static Policy::_ref_type create_policy (PolicyType type, const Any& val)
	{
		if (type == 0 || type > MAX_KNOWN_POLICY_ID)
			throw PolicyError (BAD_POLICY);
		return (creators_ [type - 1]) (val);
	}

private:
	typedef Policy::_ref_type (*Creator) (const Any&);

	static const Creator creators_ [MAX_KNOWN_POLICY_ID];
};

}
}

#endif