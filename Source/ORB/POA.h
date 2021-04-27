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
#ifndef NIRVANA_ORB_CORE_POA_H_
#define NIRVANA_ORB_CORE_POA_H_

#include "ServantBase.h"
#include <CORBA/POA_s.h>
#include "../parallel-hashmap/parallel_hashmap/phmap.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

extern PortableServer::POA::_ref_type g_root_POA; // Temporary solution

class POA :
	public Servant <POA, PortableServer::POA>
{
public:
	POA ()
	{}

	~POA ()
	{}

	static Type <String>::ABI_ret _activate_object (Bridge <PortableServer::POA>* obj, Interface* servant, Interface* env)
	{
		try {
			return Type <String>::ret (_implementation (obj).activate_object (Type <Object>::in (servant)));
		} catch (const Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return Type <String>::ABI_ret ();
	}

	String activate_object (Object::_ptr_type proxy)
	{
		if (active_object_map_.empty ())
			_add_ref ();
		std::pair <AOM::iterator, bool> ins = active_object_map_.emplace (std::to_string ((uintptr_t)&proxy), 
			Object::_ref_type (proxy));
		if (!ins.second)
			throw PortableServer::POA::ServantAlreadyActive ();
		return ins.first->first;
	}

	void deactivate_object (const String& oid)
	{
		if (!active_object_map_.erase (oid))
			throw PortableServer::POA::ObjectNotActive ();
		if (active_object_map_.empty ())
			_remove_ref ();
	}

private:
	typedef phmap::flat_hash_map <String, Object::_ref_type> AOM;
	AOM active_object_map_;
};

}
}
}

#endif
