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

#include "Reference.h"
#include <CORBA/IOP.h>

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

inline void sort (IOP::TaggedComponentSeq& components) NIRVANA_NOEXCEPT
{
	std::sort (components.begin (), components.end (), ComponentPred ());
}

IOP::TaggedComponentSeq::const_iterator find (const IOP::TaggedComponentSeq& components,
	IOP::ComponentId id) NIRVANA_NOEXCEPT;

class Domain;

/// Base for remote references.
class ReferenceRemote :
	public Reference
{
public:
	ReferenceRemote (const OctetSeq& addr, servant_reference <Domain>&& domain, const IOP::ObjectKey& object_key,
		const IDL::String& primary_iid, ULong ORB_type, const IOP::TaggedComponentSeq& components);
	~ReferenceRemote ();

	virtual void marshal (StreamOut& out) const override;
	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, unsigned flags) override;

protected:
	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;

private:
	const OctetSeq& address_;
	servant_reference <Domain> domain_;
	const IOP::ObjectKey object_key_;
};

}
}

#endif
