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
#include "DomainRemote.h"
#include "../Binder.h"

namespace CORBA {

using namespace Internal;

namespace Core {

void DomainRemote::destroy () NIRVANA_NOEXCEPT
{
	Nirvana::Core::Binder::singleton ().erase_domain (*this);
}

IORequest::_ref_type DomainRemote::create_request (const IOP::ObjectKey& object_key,
	const Operation& metadata, unsigned flags)
{
	throw NO_IMPLEMENT ();
}

void DomainRemote::post_DGC_ref_send (TimeBase::TimeT send_time, ReferenceSet& references)
{
	if (flags () & GARBAGE_COLLECTION)
		Domain::post_DGC_ref_send (send_time, references);
	else {
		for (auto& ref : references) {
			owned_references_.emplace (std::move (ref));
		}
	}
}

}
}
