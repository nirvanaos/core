/// \file
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
#ifndef NIRVANA_ORB_CORE_REFERENCEREMOTE_H_
#define NIRVANA_ORB_CORE_REFERENCEREMOTE_H_
#pragma once

#include "Domain.h"
#include <CORBA/IOP.h>
#include "ESIOP.h"
#include "StreamInEncap.h"
#include "tagged_seq.h"

namespace CORBA {
namespace Core {

/// Base for remote references.
class ReferenceRemote :
	public Reference,
	public Nirvana::Core::BinderObject
{
public:
	using Nirvana::Core::BinderObject::operator new;
	using Nirvana::Core::BinderObject::operator delete;

	ReferenceRemote (const OctetSeq& addr, servant_reference <Domain>&& domain,
		const IOP::ObjectKey& object_key, const IDL::String& primary_iid, ULong ORB_type,
		const IOP::TaggedComponentSeq& components);
	~ReferenceRemote ();

	virtual ReferenceRef marshal (StreamOut& out) override;
	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, unsigned flags,
		Internal::RequestCallback::_ptr_type callback) override;

	const Char* set_object_name (const Char* name)
	{
		object_name_ = name;
		return object_name_.c_str ();
	}

	Domain& domain () const NIRVANA_NOEXCEPT
	{
		return *domain_;
	}

	const IOP::ObjectKey& object_key () const NIRVANA_NOEXCEPT
	{
		return object_key_;
	}

	bool unconfirmed () const NIRVANA_NOEXCEPT
	{
		return DGC_key_ && DGC_key_->unconfirmed ();
	}

	Domain::DGC_RefKey* DGC_key () const NIRVANA_NOEXCEPT
	{
		return DGC_key_;
	}

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;

private:
	static unsigned get_flags (ULong ORB_type, const IOP::TaggedComponentSeq& components)
	{
		if (ESIOP::ORB_TYPE == ORB_type) {
			auto p = binary_search (components, ESIOP::TAG_FLAGS);
			if (p) {
				Octet flags;
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (p->component_data ()));
				stm.read (1, 1, &flags);
				if (stm.end ())
					throw INV_OBJREF ();
				return flags & GARBAGE_COLLECTION;
			}
		}
		return 0;
	}

private:
	const OctetSeq& address_;
	servant_reference <Domain> domain_;
	const IOP::ObjectKey object_key_;
	IDL::String object_name_;
	Domain::DGC_RefKey* DGC_key_;
};

typedef servant_reference <ReferenceRemote> ReferenceRemoteRef;

}
}

#endif
