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
#include "IDL/GIOP.h"
#include "ESIOP.h"
#include "StreamIn.h"
#include "../SkipList.h"
#include "../ExecDomain.h"

namespace CORBA {
namespace Core {

/// Incoming requests manager
class IncomingRequests
{
public:
	static void initialize ()
	{
		request_map_.construct ();
	}

	static void terminate ()
	{
		request_map_.destruct ();
	}

	template <typename IP>
	struct InetAddr
	{
		InetAddr (IP ip, uint16_t p) :
			port (p),
			ip_address (ip)
		{}

		bool operator < (const InetAddr& rhs) const NIRVANA_NOEXCEPT
		{
			if (ip_address < rhs.ip_address)
				return true;
			else if (ip_address > rhs.ip_address)
				return false;
			else
				return port < rhs.port;
		}

		uint16_t port;
		IP ip_address;
	};

	typedef InetAddr <uint32_t> Inet;
	typedef InetAddr <uint64_t> Inet6;

	template <class Addr>
	static void receive (const Addr& source, StreamIn& message);

private:
	// The client address
	struct ClientAddr
	{
		enum class Family : uint16_t
		{
			ESIOP,
			INET,
			INET6
		}
		family;

		union Address
		{
			Address (Nirvana::ESIOP::ProtDomainId addr) :
				esiop (addr)
			{}

			Address (const Inet& addr) :
				inet (addr)
			{}

			Address (const Inet6& addr) :
				inet6 (addr)
			{}

			// family == ESIOP
			Nirvana::ESIOP::ProtDomainId esiop;

			// family == INET
			Inet inet;

			// family == INET6
			Inet6 inet6;
		}
		address;

		ClientAddr (Nirvana::ESIOP::ProtDomainId client) :
			family (Family::ESIOP),
			address (client)
		{}

		ClientAddr (const Inet& client) :
			family (Family::INET),
			address (client)
		{}

		ClientAddr (const Inet6& client) :
			family (Family::INET6),
			address (client)
		{}

		bool operator < (const ClientAddr& rhs) const NIRVANA_NOEXCEPT
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
	};

	struct RequestKey : ClientAddr
	{
		uint32_t request_id;

		bool operator < (const RequestKey& rhs) const NIRVANA_NOEXCEPT
		{
			if (request_id < rhs.request_id)
				return true;
			else if (request_id > rhs.request_id)
				return false;
			else
				return ClientAddr::operator < (rhs);
		}
	};

	struct RequestVal : RequestKey
	{
		Nirvana::Core::CoreRef <Nirvana::Core::ExecDomain> exec_domain;
	};

	typedef Nirvana::Core::SkipList <RequestVal, 10> RequestMap;

	static void receive (const ClientAddr& source, StreamIn& message);

	static Nirvana::Core::StaticallyAllocated <RequestMap> request_map_;
};

template <class Addr> inline
void IncomingRequests::receive (const Addr& source, StreamIn& message)
{
	receive (ClientAddr (source), message);
}

}
}

#endif
