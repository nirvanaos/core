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
#include "NamingContextDefault.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

servant_reference <NamingContextDefault> NamingContextDefault::create ()
{
	return make_reference <NamingContextDefault> ();
}

void NamingContextDefault::add_link (const NamingContextImpl& parent)
{
	links_.insert (&parent);
}

bool NamingContextDefault::remove_link (const NamingContextImpl& parent)
{
	if (links_.size () > 1) {
		bool garbage = true;
		for (auto p : links_) {
			ContextSet parents;
			parents.insert (this);
			if (p != &parent && !p->is_cyclic (parents)) {
				garbage = false;
				break;
			}
		}
		if (garbage)
			return false;
	}
	links_.erase (&parent);
	return true;
}

bool NamingContextDefault::is_cyclic (ContextSet& parents) const
{
	if (links_.empty ())
		return false;
	if (!parents.insert (this).second)
		return true;
	for (auto p : links_) {
		if (!p->is_cyclic (parents))
			return false;
	}
	return true;
}

}
}
