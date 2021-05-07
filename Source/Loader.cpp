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

using namespace std;

namespace Nirvana {
namespace Core {

Loader Loader::singleton_;

CoreRef <Module> Loader::load (const string& name, bool singleton)
{
	CoreString name_copy (name.c_str (), name.length ());
	SYNC_BEGIN (&singleton_.sync_domain_);
	auto ins = singleton_.map_.emplace (piecewise_construct, forward_as_tuple (move (name_copy)), make_tuple ());
	if (ins.second) {
		try {
			return &ins.first->second->construct (ref (ins.first->first), singleton);
		} catch (...) {
			singleton_.map_.erase (ins.first);
			throw;
		}
	} else
		return &ins.first->second->get ();
	SYNC_END ();
}

}
}
