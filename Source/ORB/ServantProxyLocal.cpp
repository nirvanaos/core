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
#include "ServantProxyLocal.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

void ServantProxyLocal::_add_ref ()
{
	RefCntProxy::IntegralType cnt = ref_cnt_.increment_seq ();
	if (1 == cnt)
		add_ref_servant ();
}

Boolean ServantProxyLocal::non_existent ()
{
	return servant ()->_non_existent ();
}

ReferenceRef ServantProxyLocal::get_reference ()
{
	throw MARSHAL (MAKE_OMG_MINOR (4)); // Attempt to marshal Local object.
}

Policy::_ref_type ServantProxyLocal::_get_policy (PolicyType policy_type)
{
	throw NO_IMPLEMENT (MAKE_OMG_MINOR (8));
}

DomainManagersList ServantProxyLocal::_get_domain_managers ()
{
	throw NO_IMPLEMENT (MAKE_OMG_MINOR (8));
}

}
}
