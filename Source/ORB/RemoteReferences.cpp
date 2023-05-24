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
#include "RemoteReferences.h"
#include "ReferenceRemote.h"
#include "../Binder.h"
#include "DomainProt.h"
#include "DomainRemote.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

std::pair <RemoteReferences <Binder::Allocator>::References::iterator, bool>
RemoteReferences <Binder::Allocator>::emplace_reference (OctetSeq&& addr)
{
	return references_.emplace (std::move (addr), Reference::DEADLINE_MAX);
}

RemoteReferences <Binder::Allocator>::RefPtr RemoteReferences <Binder::Allocator>::make_reference (
	const OctetSeq& addr, servant_reference <Domain>&& domain, const IOP::ObjectKey& object_key,
	const IDL::String& primary_iid, ULong ORB_type, const IOP::TaggedComponentSeq& components)
{
	return RefPtr (new ReferenceRemote (addr, std::move (domain), object_key, primary_iid, ORB_type, components));
}

template <>
servant_reference <Domain> RemoteReferences <Binder::Allocator>::get_domain (const DomainAddress& domain)
{
	if (domain.family == DomainAddress::Family::ESIOP)
		return prot_domains_.get (domain.address.esiop);
	else
		throw NO_IMPLEMENT (); // TODO: Implement
}

template <>
servant_reference <Domain> RemoteReferences <Binder::Allocator>::find_domain (
	const DomainAddress& domain) const
{
	if (domain.family == DomainAddress::Family::ESIOP)
		return prot_domains_.find (domain.address.esiop);
	else
		throw NO_IMPLEMENT (); // TODO: Implement
}

}
}
