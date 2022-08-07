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
#ifndef NIRVANA_ORB_CORE_INCOMINGREQUESTS_H_
#define NIRVANA_ORB_CORE_INCOMINGREQUESTS_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "ESIOP.h"
#include "RequestIn.h"
#include "../SkipList.h"
#include "../ExecDomain.h"
#include "IDL/GIOP.h"

namespace CORBA {
namespace Core {

/// Internet address
/// 
/// \tparam U IP address type
template <typename U>
struct IP
{
	/// Construct
	/// 
	/// \param h Host address.
	/// \param p Port number.
	IP (U h, uint16_t p) :
		port (p),
		host (h)
	{}

	bool operator < (const IP& rhs) const NIRVANA_NOEXCEPT
	{
		if (host < rhs.host)
			return true;
		else if (host > rhs.host)
			return false;
		else
			return port < rhs.port;
	}

	uint16_t port;
	U host;
};

/// IPv4
typedef IP <uint32_t> IPv4;

/// IPv6
typedef IP <uint64_t> IPv6;

/// A client address.
struct ClientAddress
{
	enum class Family : uint16_t
	{
		ESIOP,
		INET,
		INET6
	};

	ClientAddress (Nirvana::ESIOP::ProtDomainId client) :
		family (Family::ESIOP),
		address (client)
	{}

	ClientAddress (const IPv4& client) :
		family (Family::INET),
		address (client)
	{}

	ClientAddress (const IPv6& client) :
		family (Family::INET6),
		address (client)
	{}

	bool operator < (const ClientAddress& rhs) const NIRVANA_NOEXCEPT
	{
		if (family < rhs.family)
			return true;
		else if (family > rhs.family)
			return false;
		else {
			switch (family) {
				case Family::ESIOP:
					return address.esiop < rhs.address.esiop;

				case Family::INET:
					return address.inet < rhs.address.inet;

				default:
					return address.inet6 < rhs.address.inet6;
			}
		}
	}

	Family family;

	union Address
	{
		Address (Nirvana::ESIOP::ProtDomainId addr) :
			esiop (addr)
		{}

		Address (const IPv4& addr) :
			inet (addr)
		{}

		Address (const IPv6& addr) :
			inet6 (addr)
		{}

		// family == ESIOP
		Nirvana::ESIOP::ProtDomainId esiop;

		// family == INET
		IPv4 inet;

		// family == INET6
		IPv6 inet6;
	}
	address;
};

/// Incoming requests manager
class IncomingRequests
{
	static const unsigned SKIP_LIST_LEVELS = 10; // TODO: config.h

public:
	/// Called on system startup
	static void initialize ()
	{
		map_.construct ();
	}

	/// Called on system shutdown
	static void terminate ()
	{
		map_.destruct ();
	}

	/// Recieve incoming request.
	/// 
	/// \param source The client address.
	/// \param rq The input request.
	static void receive (const ClientAddress& source, RequestIn& rq);

	/// Cancel incoming request.
	/// 
	/// \param source The client address.
	/// \param request_id Id of request to cancel.
	static void cancel (const ClientAddress& source, uint32_t request_id) NIRVANA_NOEXCEPT;

private:
	struct RequestKey : ClientAddress
	{
		RequestKey (const ClientAddress& addr, uint32_t rq_id) :
			ClientAddress (addr),
			request_id (rq_id)
		{}

		bool operator < (const RequestKey& rhs) const NIRVANA_NOEXCEPT
		{
			if (request_id < rhs.request_id)
				return true;
			else if (request_id > rhs.request_id)
				return false;
			else
				return ClientAddress::operator < (rhs);
		}

		uint32_t request_id;
	};

	struct RequestVal : RequestKey
	{
		RequestVal (const ClientAddress& addr, uint32_t rq_id, Nirvana::Core::ExecDomain* ed) :
			RequestKey (addr, rq_id),
			exec_domain (ed)
		{}

		Nirvana::Core::CoreRef <Nirvana::Core::ExecDomain> exec_domain;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	static Nirvana::Core::StaticallyAllocated <RequestMap> map_;
};

}
}

#endif
