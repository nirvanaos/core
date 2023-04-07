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
#include <boost/preprocessor/repetition/repeat.hpp>
#include <CORBA/FT_s.h>
#include <CORBA/BiDirPolicy_s.h>
#include "StreamInEncap.h"
#include "StreamOutEncap.h"
#include "../ExecDomain.h"

#define POLICY_FUNCTIONS(z, n, d) &PolicyImpl <n + 1>::functions_,

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Policy::_ref_type PolicyUnsupported::create (const Any&)
{
	throw PolicyError (UNSUPPORTED_POLICY);
}

PolicyFactory::Functions PolicyUnsupported::functions_ = { create, nullptr, nullptr };

DEFINE_POLICY (BiDirPolicy::BIDIRECTIONAL_POLICY_TYPE, BiDirPolicy::BidirectionalPolicy, BiDirPolicy::BidirectionalPolicyValue, value);
DEFINE_POLICY (FT::HEARTBEAT_POLICY, FT::HeartbeatPolicy, FT::HeartbeatPolicyValue, heartbeat_policy_value);
DEFINE_POLICY (FT::HEARTBEAT_ENABLED_POLICY, FT::HeartbeatEnabledPolicy, bool, heartbeat_enabled_policy_value);

const PolicyFactory::Functions* const PolicyFactory::functions_ [MAX_KNOWN_POLICY_ID] = {
	BOOST_PP_REPEAT (MAX_KNOWN_POLICY_ID, POLICY_FUNCTIONS, 0)
};

void PolicyFactory::read (const Messaging::PolicyValueSeq& in, PolicyMap& policies)
{
	SYNC_BEGIN (g_core_free_sync_context, &ExecDomain::current ().mem_context ())
		for (const Messaging::PolicyValue& val : in) {
			const Functions* f = functions (val.ptype ());
			if (f && f->read) {
				ImplStatic <StreamInEncap> stm (std::ref (val.pvalue ()));
				policies.emplace (val.ptype (), (f->read) (stm));
			}
		}
	SYNC_END ()
}

void PolicyFactory::write (const PolicyMap& policies, Messaging::PolicyValueSeq& out)
{
	for (const auto& policy : policies) {
		const Functions* f = functions (policy.first);
		assert (f && f->write);
		ImplStatic <StreamOutEncap> stm;
		(f->write) (policy.second, stm);
		out.emplace_back (policy.first, std::move (stm.data ()));
	}
}

}
}
