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
#include "Nirvana_policies.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

ReferenceRemote::ReferenceRemote (const OctetSeq& addr, servant_reference <Domain>&& domain,
	const IOP::ObjectKey& object_key, Internal::String_in primary_iid,
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
		bool gc;
		if (policies_->get_value <Nirvana::DGC_POLICY_TYPE> (gc) && gc) {
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

void ReferenceRemote::_add_ref () noexcept
{
	ref_cnt_.increment ();
}

void ReferenceRemote::_remove_ref ()
{
	if (!ref_cnt_.decrement ()) {
		SyncContext& sc = Binder::singleton ().sync_domain ();
		if (&SyncContext::current () == &sc) {
			Binder::singleton ().erase_reference (address_, object_name_.empty () ? nullptr : object_name_.c_str ());
			delete this;
		} else
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
	CallbackRef&& callback)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags, std::move (callback));

	check_create_request (op, flags);

	if (callback && !(flags & Internal::IORequest::RESPONSE_EXPECTED))
		throw BAD_PARAM ();

	return domain_->create_request (flags, object_key_, operation_metadata (op), this, std::move (callback));
}

DomainManagersList ReferenceRemote::_get_domain_managers ()
{
	throw NO_IMPLEMENT ();
}

Boolean ReferenceRemote::_is_equivalent (Object::_ptr_type other_object) const noexcept
{
	if (!other_object)
		return false;

	if (ProxyManager::_is_equivalent (other_object))
		return true;

	Reference* ref = ProxyManager::cast (other_object)->to_reference ();

	if (!ref || (ref->flags () & LOCAL))
		return false;

	const ReferenceRemote& other = static_cast <ReferenceRemote&> (*ref);
	return domain_ == other.domain_ && object_key_ == other.object_key_;
}

}
}
