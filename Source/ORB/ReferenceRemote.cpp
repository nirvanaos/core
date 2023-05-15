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
#include "../Binder.h"
#include "PolicyFactory.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

IOP::TaggedComponentSeq::const_iterator find (
	const IOP::TaggedComponentSeq& components, IOP::ComponentId id) NIRVANA_NOEXCEPT
{
	IOP::TaggedComponentSeq::const_iterator it = std::lower_bound (components.begin (), components.end (),
		id, ComponentPred ());
	if (it != components.end () && it->tag () == id)
		return it;
	else
		return components.end ();
}

ReferenceRemote::ReferenceRemote (const OctetSeq& addr, servant_reference <Domain>&& domain,
	const IOP::ObjectKey& object_key, const IDL::String& primary_iid,
	ULong ORB_type, const IOP::TaggedComponentSeq& components) :
	Reference (primary_iid, get_flags (ORB_type, components)),
	address_ (addr),
	domain_ (std::move (domain)),
	object_key_ (object_key),
	earliest_release_time_ (0)
{
	auto it = find (components, IOP::TAG_POLICIES);
	if (it != components.end ()) {
		PolicyMap policies;
		PolicyFactory::read (it->component_data (), policies);
		if (!policies.empty ())
			domain_manager_ = make_reference <Core::DomainManager> (std::move (policies));
	}
	if ((flags () & GARBAGE_COLLECTION) && (domain_->flags () & Domain::GARBAGE_COLLECTION))
		domain_->on_DGC_reference_unmarshal (object_key_);
}

ReferenceRemote::~ReferenceRemote ()
{
	if ((flags () & GARBAGE_COLLECTION) && (domain_->flags () & Domain::GARBAGE_COLLECTION))
		domain_->on_DGC_reference_delete (object_key_);
}

void ReferenceRemote::_add_ref () NIRVANA_NOEXCEPT
{
	ref_cnt_.increment ();
}

void ReferenceRemote::_remove_ref () NIRVANA_NOEXCEPT
{
	if (!ref_cnt_.decrement_seq ()) {
		SyncContext& sc = Binder::singleton ().sync_domain ();
		if (&SyncContext::current () == &sc)
			Binder::singleton ().erase_reference (address_, object_name_.empty () ? nullptr : object_name_.c_str ());
		else
			GarbageCollector::schedule (*this, sc);
	}
}

void ReferenceRemote::marshal (StreamOut& out) const
{
	out.write_string_c (primary_interface_id ());
	out.write_c (4, address_.size (), address_.data ());
}

IORequest::_ref_type ReferenceRemote::create_request (OperationIndex op, unsigned flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	check_create_request (op, flags);

	return domain_->create_request (object_key_, operation_metadata (op), flags);
}

}
}
