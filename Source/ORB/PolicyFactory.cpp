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
#include "../pch.h"
#include "PolicyFactory.h"
#include "PortableServer_policies.h"
#include "BiDirPolicy_policies.h"
#include "Messaging_policies.h"
#include "FT_policies.h"
#include "Nirvana_policies.h"
#include <boost/preprocessor/repetition/repeat.hpp>
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

const PolicyFactory::Functions* const PolicyFactory::ORB_policies_ [MAX_ORB_POLICY_ID] = {
	BOOST_PP_REPEAT (MAX_ORB_POLICY_ID, POLICY_FUNCTIONS, 0)
};

const PolicyFactory::Functions* PolicyFactory::functions (PolicyType type) noexcept
{
	if (0 == (type & 0xFFFFF000)) {
		if (type == 0 || type > MAX_ORB_POLICY_ID)
			return nullptr;
		return ORB_policies_ [type - 1];
	} else if (type == Nirvana::DGC_POLICY_TYPE)
		return &PolicyImpl <Nirvana::DGC_POLICY_TYPE>::functions_;
	else
		return nullptr;
}

void PolicyFactory::write (Policy::_ptr_type pol, StreamOut& out)
{
	const Functions* f = functions (pol->policy_type ());
	if (f && f->write)
		(f->write) (pol, out);
	else
		throw PolicyError (BAD_POLICY);
}

Policy::_ref_type PolicyFactory::create (PolicyType type, const OctetSeq& data)
{
	const Functions* f = functions (type);
	if (f && f->read) {
		ImplStatic <StreamInEncap> stm (std::ref (data));
		return (f->read) (stm);
	} else
		throw PolicyError (BAD_POLICY);
}

}
}
