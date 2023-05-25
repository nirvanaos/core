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

#include "ProtDomains.h"
#include "RemoteDomains.h"
#include "ReferenceRemote.h"
#include "HashOctetSeq.h"
#include "RequestIn.h"
#include <CORBA/I_var.h>

namespace CORBA {
namespace Core {

class RemoteReferences
{
	typedef std::unique_ptr <ReferenceRemote> RefPtr;
	typedef Nirvana::Core::WaitableRef <RefPtr> RefVal;
	typedef OctetSeq RefKey;
	typedef Nirvana::Core::MapUnorderedStable <RefKey, RefVal, std::hash <RefKey>,
		std::equal_to <RefKey>, Nirvana::Core::BinderMemory::Allocator> References;

public:
	RemoteReferences ()
	{}

	template <class EndPoint>
	Object::_ref_type unmarshal (const EndPoint& domain, const IDL::String& iid,
		const IOP::TaggedProfileSeq& addr, const IOP::ObjectKey& object_key, ULong ORB_type,
		const IOP::TaggedComponentSeq& components, ReferenceRemoteRef& unconfirmed)
	{
		Nirvana::Core::ImplStatic <StreamOutEncap> stm (true);
		stm.write_tagged (addr);
		auto ins = emplace_reference (stm.data ());
		typename References::reference entry = *ins.first;
		if (ins.second) {
			try {
				RefPtr p (make_reference (ins.first->first, get_domain (domain),
					object_key, iid, ORB_type, components));
				if (p->unconfirmed ())
					unconfirmed = p.get ();
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

	ESIOP::ProtDomains& prot_domains () NIRVANA_NOEXCEPT
	{
		return prot_domains_;
	}

	RemoteDomains& remote_domains () NIRVANA_NOEXCEPT
	{
		return remote_domains_;
	}

	void erase (const RefKey& ref) NIRVANA_NOEXCEPT
	{
		references_.erase (ref);
	}

	servant_reference <Domain> get_domain (const DomainAddress& domain);

	servant_reference <ESIOP::DomainProt> get_domain (ESIOP::ProtDomainId domain)
	{
		return prot_domains_.get (domain);
	}

	servant_reference <DomainRemote> get_domain (const IIOP::ListenPoint& lp)
	{
		return remote_domains_.get (lp);
	}

	void heartbeat (const DomainAddress& domain)
	{
		auto p = find_domain (domain);
		if (p)
			p->simple_ping ();
	}

	void complex_ping (RequestIn& rq)
	{
		get_domain (rq.key ())->complex_ping (rq._get_ptr ());
	}

private:
	std::pair <typename References::iterator, bool> emplace_reference (const OctetSeq& addr);
	static std::unique_ptr <ReferenceRemote> make_reference (const OctetSeq& addr,
		servant_reference <Domain>&& domain, const IOP::ObjectKey& object_key,
		const IDL::String& primary_iid, ULong ORB_type, const IOP::TaggedComponentSeq& components);

	servant_reference <Domain> find_domain (const DomainAddress& domain) const noexcept
	{
		if (domain.family == DomainAddress::Family::ESIOP)
			return prot_domains_.find (domain.address.esiop);
		else
			return nullptr; // TODO: Implement
	}

private:
	ESIOP::ProtDomains prot_domains_;
	RemoteDomains remote_domains_;
	References references_;
};

}
}

#endif
