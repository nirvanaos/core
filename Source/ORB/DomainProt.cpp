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
#include "DomainProt.h"
#include "../Binder.h"
#include "RequestOutESIOP.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace CORBA;

namespace ESIOP {

void DomainProt::destroy () NIRVANA_NOEXCEPT
{
	Binder::singleton ().remote_references ().prot_domains ().erase (id_);
}

CORBA::Internal::IORequest::_ref_type DomainProt::create_request (const IOP::ObjectKey& object_key,
	const Internal::Operation& metadata, unsigned response_flags)
{
	return make_pseudo <RequestOut> (std::ref (*this), std::ref (object_key), std::ref (metadata),
		response_flags, IOP::ServiceContextList ());
}

}