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
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

ObjectId POA_ImplicitUnique::activate_object (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	ObjectId oid = POA_SystemId::unique_id (object2proxy (p_servant));
	if (!AOM_insert (oid, p_servant))
		throw ServantAlreadyActive ();
	return oid;
}

void POA_ImplicitUnique::activate_object_with_id (const ObjectId& oid, CORBA::Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	if (oid != POA_SystemId::unique_id (object2proxy (p_servant)))
		throw BAD_PARAM ();
	if (!AOM_insert (oid, p_servant))
		throw ServantAlreadyActive ();
}

ObjectId POA_ImplicitUnique::servant_to_id (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	ObjectId oid = POA_SystemId::unique_id (object2proxy (p_servant));
	AOM_insert (oid, p_servant);
	return oid;
}

Object::_ref_type POA_ImplicitUnique::servant_to_reference (Object::_ptr_type p_servant)
{
	if (!p_servant)
		throw BAD_PARAM ();

	AOM_insert (POA_SystemId::unique_id (object2proxy (p_servant)), p_servant);

	return p_servant;
}

bool POA_ImplicitUnique::AOM_insert (const ObjectId& oid, Object::_ptr_type p_servant)
{
	assert (p_servant);
	auto ins = active_object_map_.emplace (oid, p_servant);
	if (ins.second) {
		try {
			ActivationKeyRef key = ActivationKeyRef::create <ActivationKeyImpl> ();
			get_path (key->adapter_path);
			key->object_id = oid;
			object2proxy (p_servant)->activate (std::move (key));
		} catch (...) {
			active_object_map_.erase (ins.first);
			throw;
		}
	}
	return ins.second;
}

}
}
