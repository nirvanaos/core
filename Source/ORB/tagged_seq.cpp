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
#include "../pch.h"
#include "tagged_seq.h"
#include <algorithm>

namespace CORBA {
namespace Core {

struct ComponentPred
{
	bool operator () (const IOP::TaggedComponent& l, const IOP::TaggedComponent& r) const
	{
		return l.tag () < r.tag ();
	}

	bool operator () (const IOP::ComponentId l, const IOP::TaggedComponent& r) const
	{
		return l < r.tag ();
	}

	bool operator () (const IOP::TaggedComponent& l, const IOP::ComponentId& r) const
	{
		return l.tag () < r;
	}
};

void sort (IOP::TaggedComponentSeq& seq) noexcept
{
	std::sort (seq.begin (), seq.end (), ComponentPred ());
}

const IOP::TaggedComponent* binary_search (
	const IOP::TaggedComponentSeq& seq, IOP::ComponentId id) noexcept
{
	IOP::TaggedComponentSeq::const_iterator it = std::lower_bound (seq.begin (), seq.end (),
		id, ComponentPred ());
	if (it != seq.end () && it->tag () == id)
		return &*it;
	else
		return nullptr;
}

}
}

