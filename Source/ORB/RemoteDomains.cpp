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
#include "RemoteDomains.h"
#include <Nirvana/Hash.h>
#include "../Binder.h"

namespace std {

size_t hash <IIOP::ListenPoint>::operator () (const IIOP::ListenPoint& lp) const noexcept
{
	size_t h = Nirvana::Hash::hash_bytes (lp.host ().data (), lp.host ().size ());
	CORBA::UShort port = lp.port ();
	return Nirvana::Hash::append_bytes (h, &port, sizeof (port));
}

}

namespace CORBA {
namespace Core {

servant_reference <DomainRemote> RemoteDomains::get (const IIOP::ListenPoint& lp)
{
	auto ins = listen_points_.emplace (std::ref (lp), nullptr);
	if (ins.second) {
		try {
			if (domains_.empty ())
				Nirvana::Core::Binder::singleton ().start_domains_housekeeping ();
			domains_.emplace_back ();
			DomainRemote& domain = domains_.back ();
			domain.add_listen_point (ins.first->first);
			ins.first->second = &domain;
		} catch (...) {
			domains_.pop_back ();
			listen_points_.erase (ins.first);
			throw;
		}
	}
	return ins.first->second;
}

}
}
