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
#include "PolicyFactory.h"
#include "PortableServer_policies.h"
#include "BiDirPolicy_s.h"
#include <boost/preprocessor/repetition/repeat.hpp>

#define CREATOR(z, n, d) PolicyImpl <n + 1>::create_policy,

namespace CORBA {
namespace Core {

DEFINE_POLICY (BiDirPolicy::BIDIRECTIONAL_POLICY_TYPE, BiDirPolicy::BidirectionalPolicy, BiDirPolicy::BidirectionalPolicyValue, value);

const PolicyFactory::Creator PolicyFactory::creators_ [MAX_KNOWN_POLICY_ID] = {
	BOOST_PP_REPEAT (MAX_KNOWN_POLICY_ID, CREATOR, 0)
};

}
}
