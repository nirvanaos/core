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
	static const TimeBase::TimeT REQUEST_LATENCY = 10 * TimeBase::MILLISECOND;
	static const TimeBase::TimeT HEARTBEAT_INTERVAL = 30 * TimeBase::SECOND;
	static const TimeBase::TimeT HEARTBEAT_TIMEOUT = 2 * HEARTBEAT_INTERVAL;

	DomainProt (ESIOP::ProtDomainId id) :
		CORBA::Core::Domain (GARBAGE_COLLECTION | HEARTBEAT_IN | HEARTBEAT_OUT,
			REQUEST_LATENCY, HEARTBEAT_INTERVAL, HEARTBEAT_TIMEOUT),
		OtherDomain (id)
	{}
	
	~DomainProt ();

	virtual CORBA::Internal::IORequest::_ref_type create_request (const IOP::ObjectKey& object_key,
		const CORBA::Internal::Operation& metadata, unsigned response_flags, CORBA::Internal::RequestCallback::_ptr_type callback) override;

	void shutdown () NIRVANA_NOEXCEPT
	{
		make_zombie ();
		CloseConnection msg (current_domain_id ());
		try {
			send_message (&msg, sizeof (msg));
		} catch (...) {}
	}

};

}

#endif
