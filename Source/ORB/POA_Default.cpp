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
#include "POA_Default.h"
#include "POA_Root.h"

using namespace CORBA;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

CORBA::Object::_ref_type POA_Default::get_servant ()
{
	if (!servant_)
		throw NoServant ();

	return servant_;
}

void POA_Default::set_servant (CORBA::Object::_ptr_type p_servant)
{
	servant_ = p_servant;
}

Object::_ref_type POA_Default::reference_to_servant_default (bool)
{
	return servant_;
}

Object::_ref_type POA_Default::id_to_servant_default (bool)
{
	return servant_;
}

void POA_Default::serve_default (const RequestRef& request, CORBA::Core::ReferenceLocal& reference)
{
	// If the POA has the USE_DEFAULT_SERVANT policy, a default servant has been associated with the
	// POA so the POA will invoke the appropriate method on that servant. If no servant has been
	// associated with the POA, the POA raises the OBJ_ADAPTER system exception with standard minor
	// code 3.
	if (!servant_)
		throw OBJ_ADAPTER (MAKE_OMG_MINOR (3));

	servant_reference <ServantProxyObject> hold (object2proxy (servant_));
	serve_request (request, reference, *hold);
}

}
}
