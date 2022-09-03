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
#include "POA_Implicit.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

ObjectId POA_Implicit::servant_to_id (Object::_ptr_type p_servant)
{
	return activate_object (p_servant);
}

Object::_ref_type POA_Implicit::servant_to_reference (Object::_ptr_type p_servant)
{
	CoreRef <ProxyObject> proxy = get_proxy (p_servant);
	ObjectIdSys id (*proxy);
	AOM::iterator it = activate (id, std::move (proxy));
	return it->second->get_proxy ();
}

ObjectId POA_ImplicitUnique::servant_to_id (Object::_ptr_type p_servant)
{
	auto it = servant_find (p_servant);
	if (it != servant_map_.end ())
		return it->second->first.to_object_id ();
	else
		return activate_object (p_servant);
}

Object::_ref_type POA_ImplicitUnique::servant_to_reference (Object::_ptr_type p_servant)
{
	Object::_ref_type ret;
	auto it = servant_find (p_servant);
	if (it != servant_map_.end ())
		ret = it->second->second->get_proxy ();
	else {
		ServantMap::iterator it_servant = servant_add (p_servant);
		CoreRef <ProxyObject> proxy = get_proxy (p_servant);
		ObjectIdSys id (*proxy);
		ret = proxy->get_proxy ();
		AOM::iterator it_object = activate (id, std::move (proxy));
		it_servant->second = &*it_object;
	}
	return ret;
}

}
}
