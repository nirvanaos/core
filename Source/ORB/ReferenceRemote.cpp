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
#include "Domain.h"
#include "StreamOut.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

ReferenceRemote::ReferenceRemote (servant_reference <Domain>&& domain, const IOP::ObjectKey& key,
	const IOP::TaggedProfileSeq& addr, const IDL::String& primary_iid, unsigned flags) :
	Reference (primary_iid, flags),
	domain_ (std::move (domain)),
	object_key_ (key),
	address_ (addr)
{}

ReferenceRemote::~ReferenceRemote ()
{}

void ReferenceRemote::_add_ref () NIRVANA_NOEXCEPT
{
	ref_cnt_.increment ();
}

void ReferenceRemote::_remove_ref () NIRVANA_NOEXCEPT
{
	if (!ref_cnt_.decrement_seq ()) {
		RemoteReferences& service = static_cast <RemoteReferences&> (domain_->service ());
		SyncContext& sc = service.sync_domain ();
		if (&SyncContext::current () == &sc)
			service.erase (object_key_);
		else
			GarbageCollector::schedule (*this, sc);
	}
}

void ReferenceRemote::marshal (StreamOut& out) const
{
	out.write_string_c (primary_interface_id ());
	out.write_tagged (address_);
}

}
}
