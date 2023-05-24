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
#ifndef NIRVANA_ORB_CORE_REMOTEDOMAINS_H_
#define NIRVANA_ORB_CORE_REMOTEDOMAINS_H_
#pragma once

#include "../MapUnorderedStable.h"
#include <CORBA/IIOP.h>

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

class DomainRemote;

template <template <class> class Al>
class RemoteDomains
{
public:
	servant_reference <DomainRemote> get (const IIOP::ListenPoint& lp)
	{
		auto ins = listen_points_.emplace (std::ref (lp), nullptr);
		if (ins.second)
			ins.first->second = make_domain (lp);
		return ins.first->second;
	}

	void erase (const IIOP::ListenPoint& lp) noexcept
	{
		listen_points_.erase (lp);
	}

private:
	static servant_reference <DomainRemote> make_domain (const IIOP::ListenPoint& lp);

	typedef Nirvana::Core::MapUnorderedStable <IIOP::ListenPoint, servant_reference <DomainRemote>, std::hash <IIOP::ListenPoint>,
		std::equal_to <IIOP::ListenPoint>, Al> ListenPoints;

	ListenPoints listen_points_;
};

}
}

#endif
