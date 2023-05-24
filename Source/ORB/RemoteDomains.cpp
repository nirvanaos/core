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
#include "DomainRemote.h"
#include "../Binder.inl"

namespace std {

size_t hash <IIOP::ListenPoint>::operator () (const IIOP::ListenPoint& lp) const NIRVANA_NOEXCEPT
{
	size_t h = Nirvana::Hash::hash_bytes (lp.host ().data (), lp.host ().size ());
	CORBA::UShort port = lp.port ();
	return Nirvana::Hash::append_bytes (h, &port, sizeof (port));
}

}

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

servant_reference <DomainRemote> RemoteDomains <Binder::Allocator>::make_domain (const IIOP::ListenPoint& lp)
{
	return make_reference <DomainRemote> (std::ref (lp));
}

}
}
