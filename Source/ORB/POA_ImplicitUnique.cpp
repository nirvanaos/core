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
#include "POA_ImplicitUnique.h"
#include "POA_SystemId.h"

using namespace CORBA;

namespace PortableServer {
namespace Core {

ObjectId POA_ImplicitUnique::activate_object (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	auto proxy = object2servant_core (p_servant);
	POA::_ptr_type activated_POA = proxy->activated_POA ();
	if (activated_POA && activated_POA->_is_equivalent (_this ()))
		throw ServantAlreadyActive (); // This servant already activated in this POA

	ObjectId oid;
	AOM::const_iterator it = AOM_insert (p_servant, oid);

	if (!activated_POA)
		proxy->activate (_this (), it->first, true);

	return it->first;
}

ObjectId POA_ImplicitUnique::servant_to_id (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	auto proxy = object2servant_core (p_servant);
	POA::_ptr_type activated_POA = proxy->activated_POA ();
	if (activated_POA && activated_POA->_is_equivalent (_this ()))
		return *proxy->activated_id ();

	ObjectId oid;
	AOM::const_iterator it = AOM_insert (p_servant, oid);
	if (!activated_POA)
		proxy->activate (_this (), it->first, true);
	return oid;
}

Object::_ref_type POA_ImplicitUnique::servant_to_reference (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	auto proxy = object2servant_core (p_servant);
	POA::_ptr_type activated_POA = proxy->activated_POA ();
	if (!activated_POA) {
		ObjectId oid;
		AOM::const_iterator it = AOM_insert (p_servant, oid);
		proxy->activate (_this (), it->first, true);
	}
	return p_servant;
}

AOM::const_iterator POA_ImplicitUnique::AOM_insert (Object::_ptr_type p_servant, ObjectId& oid)
{
	assert (p_servant);
	auto proxy = object2servant_core (p_servant);
	oid = POA_SystemId::unique_id (proxy);
	auto ins = active_object_map_.emplace (oid, p_servant);
	assert (ins.second);
	if (!ins.second)
		throw CORBA::OBJ_ADAPTER ();
	return ins.first;
}

}
}
