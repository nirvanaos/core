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
#ifndef NIRVANA_ORB_CORE_DOMAINPROT_H_
#define NIRVANA_ORB_CORE_DOMAINPROT_H_
#pragma once

#include "Domain.h"
#include <Port/OtherDomain.h>

namespace ESIOP {

/// Other protection domain on the same system.
class DomainProt :
	public CORBA::Core::Domain,
	public OtherDomain
{
public:
	DomainProt (ESIOP::ProtDomainId id) :
		CORBA::Core::Domain (GARBAGE_COLLECTION | HEARTBEAT_IN | HEARTBEAT_OUT,
			10 * TimeBase::MILLISECOND, 30 * TimeBase::SECOND, 1 * TimeBase::MINUTE),
		OtherDomain (id),
		id_ (id)
	{}
	
	~DomainProt ()
	{}

	virtual CORBA::Internal::IORequest::_ref_type create_request (const IOP::ObjectKey& object_key,
		const CORBA::Internal::Operation& metadata, unsigned response_flags) override;

protected:
	virtual void destroy () NIRVANA_NOEXCEPT override;

private:
	ProtDomainId id_;
};

}

#endif