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
#include "Reference.h"

using namespace Nirvana::Core;
using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

namespace CORBA {
namespace Core {

Policy::_ref_type Reference::_get_policy (PolicyType policy_type)
{
	if (domain_manager_)
		return domain_manager_->get_policy (policy_type);
	else
		return Policy::_nil ();
}

DomainManagersList Reference::_get_domain_managers ()
{
	// TODO: At least one domain manager must be returned in the
	// list since by default each object is associated with at least one domain manager at creation.
	DomainManagersList ret;
	if (domain_manager_)
		ret.push_back (domain_manager_->_this ());
	return ret;
}

}
}
