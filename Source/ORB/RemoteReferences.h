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
#ifndef NIRVANA_ORB_CORE_REMOTEREFERENCES_H_
#define NIRVANA_ORB_CORE_REMOTEREFERENCES_H_
#pragma once

#include "ReferenceRemote.h"
#include "DomainsLocal.h"
#include "DomainRemote.h"
#include "HashOctetSeq.h"
#include "RequestIn.h"
#include <CORBA/IOP.h>
#include <CORBA/I_var.h>

namespace std {

template <>
struct hash <IIOP::ListenPoint>
{
	size_t operator () (const IIOP::ListenPoint& lp) const NIRVANA_NOEXCEPT;
};

template <>
struct equal_to <IIOP::ListenPoint>
{
	bool operator () (const IIOP::ListenPoint& l, const IIOP::ListenPoint& r) const NIRVANA_NOEXCEPT
	{
		return l.port () == r.port () && l.host () == r.host ();
	}
};

}

namespace CORBA {
namespace Core {

template <template <class> class Al>
class RemoteReferences
{
	typedef std::unique_ptr <ReferenceRemote> RefPtr;
	typedef Nirvana::Core::WaitableRef <RefPtr> RefVal;
	typedef OctetSeq RefKey;
	typedef Nirvana::Core::MapUnorderedStable <RefKey, RefVal, std::hash <RefKey>,
		std::equal_to <RefKey>, Al> References;

public:
	RemoteReferences ()
	{}

	template <class DomainKey>
	Object::_ref_type unmarshal (DomainKey domain, const IDL::String& iid, const IOP::TaggedProfileSeq& addr,
		const IOP::ObjectKey& object_key, ULong ORB_type, const IOP::TaggedComponentSeq& components)
	{
		Nirvana::Core::ImplStatic <StreamOutEncap> stm (true);
		stm.write_tagged (addr);
		auto ins = emplace_reference (std::move (stm.data ()));
		typename References::reference entry = *ins.first;
		if (ins.second) {
			try {
				RefPtr p (new ReferenceRemote (ins.first->first, get_domain_sync (domain),
					std::move (object_key), iid, ORB_type, components));
				Internal::I_var <Object> ret (p->get_proxy ()); // Use I_var to avoid reference counter increment
				entry.second.finish_construction (std::move (p));
				return ret;
			} catch (...) {
				entry.second.on_exception ();
				references_.erase (ins.first->first);
				throw;
			}
		} else {
			const RefPtr& p = entry.second.get ();
			p->check_primary_interface (iid);
			return p->get_proxy ();
		}
	}

	void erase_domain (ESIOP::ProtDomainId id) NIRVANA_NOEXCEPT
	{
		domains_local_.erase (id);
	}

	void erase_domain (const DomainRemote& domain) NIRVANA_NOEXCEPT
	{
		domains_remote_.erase (domain);
	}

	void erase (const RefKey& ref) NIRVANA_NOEXCEPT
	{
		references_.erase (ref);
	}

#ifndef SINGLE_DOMAIN
	servant_reference <ESIOP::DomainLocal> get_domain_sync (ESIOP::ProtDomainId id)
	{
		return domains_local_.get (id);
	}
#endif

	servant_reference <Domain> get_domain_sync (const IIOP::ListenPoint& lp)
	{
		auto ins = domains_remote_.emplace (std::ref (lp));
		DomainRemote* p = &const_cast <DomainRemote&> (*ins.first);
		if (ins.second)
			return PortableServer::Servant_var <Domain> (p);
		else
			return p;
	}

	void heartbeat (const DomainAddress& da) NIRVANA_NOEXCEPT
	{
		assert (da.family == DomainAddress::Family::ESIOP); // TODO
		auto d = domains_local_.find (da.address.esiop);
		if (d)
			d->simple_ping ();
	}

	void complex_ping (RequestIn& rq)
	{
		assert (rq.key ().family == DomainAddress::Family::ESIOP); // TODO
		auto d = get_domain_sync (rq.key ().address.esiop);
		if (d)
			d->complex_ping (rq._get_ptr ());
	}

private:
	std::pair <typename References::iterator, bool> emplace_reference (OctetSeq&& addr);

private:
#ifndef SINGLE_DOMAIN
	ESIOP::DomainsLocal <Al> domains_local_;
#endif

	typedef Nirvana::Core::SetUnorderedStable <DomainRemote, std::hash <IIOP::ListenPoint>,
		std::equal_to <IIOP::ListenPoint>, Al> DomainsRemote;

	DomainsRemote domains_remote_;

	References references_;
};

template <template <class> class Al>
std::pair <typename RemoteReferences <Al>::References::iterator, bool>
RemoteReferences <Al>::emplace_reference (OctetSeq&& addr)
{
	return references_.emplace (std::move (addr), Reference::DEADLINE_MAX);
}

}
}

#endif
