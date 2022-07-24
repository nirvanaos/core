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
#include "POA_AOM.h"

using namespace CORBA;

namespace PortableServer {
namespace Core {

void POA_AOM::destroy (bool etherealize_objects, bool wait_for_completion)
{
	Base::destroy (etherealize_objects, wait_for_completion);

	AOM tmp (std::move (active_object_map_));
	POA::_ref_type self = _this ();
	while (!tmp.empty ()) {
		auto it = tmp.begin ();
		ServantBase* proxy = object2servant_core (it->second);
		POA::_ptr_type activated_POA = proxy->activated_POA ();
		if (activated_POA && activated_POA->_is_equivalent (self))
			proxy->deactivate ();
		tmp.erase (it);
	}
}

void POA_AOM::deactivate_object (const ObjectId& oid)
{
	auto it = active_object_map_.find (oid);
	if (it == active_object_map_.end ())
		throw ObjectNotActive ();
	ServantBase* proxy = object2servant_core (it->second);
	POA::_ptr_type activated_POA = proxy->activated_POA ();
	assert (activated_POA);
	if (activated_POA && activated_POA->_is_equivalent (_this ()))
		proxy->deactivate ();
	active_object_map_.erase (it);
}

Object::_ref_type POA_AOM::id_to_reference (const ObjectId& oid)
{
	AOM::const_iterator it = active_object_map_.find (oid);
	if (it != active_object_map_.end ())
		return it->second;

	throw ObjectNotActive ();
}


}
}
