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
#ifndef NIRVANA_ORB_CORE_DOMAINMANAGER_H_
#define NIRVANA_ORB_CORE_DOMAINMANAGER_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/DomainManager_s.h>
#include <CORBA/NoDefaultPOA.h>
#include "PolicyMap.h"

namespace CORBA {
namespace Core {

class DomainManager :
	public servant_traits <CORBA::DomainManager>::Servant <DomainManager>,
	public PortableServer::NoDefaultPOA
{
public:
	using PortableServer::NoDefaultPOA::__default_POA;

	DomainManager (PolicyMap&& policies) :
		policies_ (std::move (policies))
	{}

	DomainManager (DomainManager&&) = default;

	const PolicyMap& policies () const NIRVANA_NOEXCEPT
	{
		return policies_;
	}

	Policy::_ref_type get_domain_policy (PolicyType policy_type)
	{
		Policy::_ref_type ret = get_policy (policy_type);
		if (!ret)
			throw INV_POLICY (MAKE_OMG_MINOR (2));
		return ret;
	}

	Policy::_ref_type get_policy (PolicyType policy_type) const NIRVANA_NOEXCEPT;
	
	bool add_policy (PolicyType policy_type, Policy::_ptr_type policy)
	{
		return policies_.emplace (policy->policy_type (), policy).second;
	}

private:
	PolicyMap policies_;
};

}
}

#endif
