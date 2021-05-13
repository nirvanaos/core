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
#include "Loader.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "ORB/ServantMarshaler.h"

using namespace std;

namespace Nirvana {
namespace Core {

Loader Loader::singleton_;

CoreRef <Module> Loader::load (const string& _name, bool singleton)
{
	CORBA::Internal::Core::ServantMarshaler sm (singleton_.sync_domain_);
	CORBA::Internal::Type <string>::ABI name_abi;
	CORBA::Internal::Type <string>::marshal_in (_name, sm.marshaler (), name_abi);
	SYNC_BEGIN (&singleton_.sync_domain_);
	string name;
	CORBA::Internal::Type <string>::unmarshal (name_abi, sm.unmarshaler (), name);
	if (singleton_.terminated_)
		throw_BAD_INV_ORDER ();
	auto ins = singleton_.map_.emplace (piecewise_construct, forward_as_tuple (move (name)), make_tuple ());
	if (ins.second) {
		try {
			if (singleton)
				return ins.first->second.initialize (new Singleton (ins.first->first));
			else
				return ins.first->second.initialize (new ClassLibrary (ins.first->first));
		} catch (...) {
			singleton_.map_.erase (ins.first);
			throw;
		}
	} else {
		Module* p = ins.first->second.get ();
		if (p->singleton () != singleton)
			throw_BAD_PARAM ();
		return p;
	}
	SYNC_END ();
}

}
}
