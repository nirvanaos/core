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
#include "ReferenceRemote.h"
#include "../Binder.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

ReferenceRemote::ReferenceRemote (const OctetSeq& addr, servant_reference <Domain>&& domain,
	const IOP::ObjectKey& object_key, const IDL::String& primary_iid,
	ULong ORB_type, const IOP::TaggedComponentSeq& components) :
	Reference (primary_iid, 0),
	address_ (addr),
	domain_ (std::move (domain)),
	object_key_ (object_key),
	DGC_key_ (nullptr)
{
	auto p = binary_search (components, IOP::TAG_POLICIES);
	if (p) {
		policies_ = Binder::singleton ().remote_references ().unmarshal_policies (p->component_data ());
		bool gc = false;
		if (policies_->get_value <FT::HEARTBEAT_ENABLED_POLICY> (gc) && gc) {
			flags_ |= GARBAGE_COLLECTION;
			if (domain_->flags () & Domain::GARBAGE_COLLECTION)
				DGC_key_ = domain_->on_DGC_reference_unmarshal (object_key_);
		}
	}
}

ReferenceRemote::~ReferenceRemote ()
{
	if (DGC_key_)
		domain_->on_DGC_reference_delete (*DGC_key_);
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

ReferenceRef ReferenceRemote::marshal (StreamOut& out)
{
	out.write_string_c (primary_interface_id ());
	out.write_c (4, address_.size (), address_.data ());
	return this;
}

IORequest::_ref_type ReferenceRemote::create_request (OperationIndex op, unsigned flags,
	RequestCallback::_ptr_type callback)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags, callback);

	check_create_request (op, flags);

	if (callback && !(flags & Internal::IORequest::RESPONSE_EXPECTED))
		throw BAD_PARAM ();

	return domain_->create_request (object_key_, operation_metadata (op), flags, callback);
}

DomainManagersList ReferenceRemote::_get_domain_managers ()
{
	throw NO_IMPLEMENT ();
}

}
}
