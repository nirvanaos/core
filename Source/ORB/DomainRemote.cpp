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

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

DomainRemote::~DomainRemote ()
{
	RemoteDomains& domains = Nirvana::Core::Binder::singleton ().remote_references ().remote_domains ();
	
	for (const IIOP::ListenPoint* lp : listen_points_)
		domains.erase (*lp);
}

IORequest::_ref_type DomainRemote::create_request (unsigned response_flags,
	const IOP::ObjectKey& object_key, const Operation& metadata, ReferenceRemote* ref,
	Interface::_ptr_type callback, OperationIndex op_idx)
{
	throw NO_IMPLEMENT ();
}

void DomainRemote::add_DGC_objects (ReferenceSet <Nirvana::Core::HeapAllocator>& references) noexcept
{
	assert (!(flags () & GARBAGE_COLLECTION));
	Nirvana::Core::Synchronized _sync_frame (Nirvana::Core::Binder::sync_domain (), nullptr);
	try {
		for (auto& ref : references) {
			owned_references_.emplace (std::move (ref));
		}
	} catch (...) {
		// TODO: Log
	}
}

}
}
