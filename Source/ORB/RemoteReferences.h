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

#include "Services.h"
#include "ReferenceRemote.h"
#include "DomainsLocal.h"
#include "DomainRemote.h"
#include "HashOctetSeq.h"
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

class RemoteReferences :
	public Nirvana::Core::Service
{
	typedef std::unique_ptr <ReferenceRemote> RefPtr;
	typedef Nirvana::Core::WaitableRef <RefPtr> RefVal;
	typedef IOP::ObjectKey RefKey;
	typedef Nirvana::Core::MapUnorderedStable <RefKey, RefVal, std::hash <RefKey>,
		std::equal_to <RefKey>, Nirvana::Core::UserAllocator <std::pair <RefKey, RefVal> > >
		References;

public:
	static RemoteReferences& singleton ()
	{
		return static_cast <RemoteReferences&> (*CORBA::Core::Services::bind (CORBA::Core::Services::RemoteReferences));
	}

	template <typename DomainKey>
	Object::_ref_type unmarshal (DomainKey domain, IOP::ObjectKey&& key, const IDL::String& iid, IOP::TaggedProfileSeq&& addr, unsigned flags)
	{
		SYNC_BEGIN (sync_domain (), nullptr)
			auto ins = emplace_reference (std::move (key));
			References::reference entry = *ins.first;
			if (ins.second) {
				try {
					RefPtr p (new ReferenceRemote (get_domain_sync (std::move (domain)), ins.first->first, std::move (addr), iid, flags));
					Internal::I_var <Object> ret (p->get_proxy ());
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
		SYNC_END ();
	}

	void erase (ESIOP::ProtDomainId id) NIRVANA_NOEXCEPT
	{
		domains_local_.erase (id);
	}

	void erase (const DomainRemote& domain) NIRVANA_NOEXCEPT
	{
		domains_remote_.erase (domain);
	}

	void erase (const RefKey& ref) NIRVANA_NOEXCEPT
	{
		references_.erase (ref);
	}

#ifndef SINGLE_DOMAIN
	servant_reference <DomainLocal> get_domain (ESIOP::ProtDomainId id)
	{
		SYNC_BEGIN (sync_domain (), nullptr)
			return get_domain_sync (id);
		SYNC_END ();
	}
#endif

private:
	std::pair <References::iterator, bool> emplace_reference (IOP::ObjectKey&& key);

#ifndef SINGLE_DOMAIN
	servant_reference <DomainLocal> get_domain_sync (ESIOP::ProtDomainId id)
	{
		return domains_local_.get (*this, id);
	}
#endif

	servant_reference <Domain> get_domain_sync (IIOP::ListenPoint&& lp)
	{
		auto ins = domains_remote_.emplace (std::ref (*this), std::move (lp));
		DomainRemote* p = &const_cast <DomainRemote&> (*ins.first);
		if (ins.second)
			return PortableServer::Servant_var <Domain> (p);
		else
			return p;
	}


private:
#ifndef SINGLE_DOMAIN
	DomainsLocal domains_local_;
#endif

	typedef Nirvana::Core::SetUnorderedStable <DomainRemote, std::hash <IIOP::ListenPoint>,
		std::equal_to <IIOP::ListenPoint>, Nirvana::Core::UserAllocator <DomainRemote> >
		DomainsRemote;

	DomainsRemote domains_remote_;

	References references_;
};

}
}

#endif
