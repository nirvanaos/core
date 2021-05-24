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
#include "Binder.inl"
#include "ORB/ServantMarshaler.h"

using namespace std;
using namespace CORBA::Internal;
using namespace CORBA::Internal::Core;

namespace Nirvana {
namespace Core {

Loader Loader::singleton_;

CoreRef <Module> Loader::load_impl (const string& _name, bool singleton)
{
	ServantMarshaler sm (sync_domain_);
	Type <string>::ABI name_abi;
	Type <string>::marshal_in (_name, sm.marshaler (), name_abi);
	SYNC_BEGIN (sync_domain_);
	string name;
	Type <string>::unmarshal (name_abi, sm.unmarshaler (), name);
	if (!initialized_)
		throw_INITIALIZE ();

	Chrono::Duration time = Chrono::steady_clock ();
	if (time - latest_housekeeping_ >= MODULE_UNLOAD_TIMEOUT) {
		// Housekeeping
		latest_housekeeping_ = time;
		time -= MODULE_UNLOAD_TIMEOUT;
		for (auto it = map_.begin (); it != map_.end ();) {
			Module* pm = it->second.get_constructed ();
			if (pm && pm->can_be_unloaded (time))
				it = map_.erase (it);
			else
				++it;
		}
	}

	auto ins = map_.emplace (piecewise_construct, forward_as_tuple (move (name)), make_tuple ());
	if (ins.second) {
		try {
			if (singleton)
				return ins.first->second.initialize (new Singleton (ins.first->first));
			else
				return ins.first->second.initialize (new ClassLibrary (ins.first->first));
		} catch (...) {
			map_.erase (ins.first);
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
