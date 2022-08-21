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
#ifndef NIRVANA_ORB_CORE_POA_ROOT_H_
#define NIRVANA_ORB_CORE_POA_ROOT_H_
#pragma once

#include "POA_ImplicitUnique.h"
#include <algorithm>

namespace PortableServer {
namespace Core {

class POA_Root :
	public POA_ImplicitUnique
{
public:
	POA_Root () :
		POA_ImplicitUnique (POAManager::_nil ())
	{}

	~POA_Root ()
	{}

	virtual IDL::String the_name () const override
	{
		return "RootPOA";
	}

	virtual CORBA::OctetSeq id () const override
	{
		return CORBA::OctetSeq ();
	}

	static CORBA::Object::_ref_type get_object (const CORBA::Core::ObjectKey& key)
	{
		POA::_ref_type root = ServantBase::_default_POA ();
		const CORBA::Core::LocalObject* proxy = get_proxy (root);
		POA_Base* adapter = get_implementation (proxy);
		assert (adapter);
		const char* path = key.POA_path.c_str ();
		const char* end = path + key.POA_path.size ();
		SYNC_BEGIN (proxy->sync_context (), nullptr);
		while (path != end) {
			const char* name_end = std::find (path, end, '/');
			if (name_end == end) {
				adapter = &adapter->find_child (CORBA::Internal::StringBase <char> (path, name_end - path), true);
				path = end;
			} else {
				adapter = &adapter->find_child (IDL::String (path, name_end - path), true);
				path = name_end + 1;
			}
		}
		return adapter->id_to_reference (key.object_id);
		SYNC_END ();
	}
};

}
}

#endif
