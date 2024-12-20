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
#include "../pch.h"
#include "DomainProt.h"
#include "../Binder.h"
#include "RequestOutESIOP.h"
#include "RequestOutSync.h"
#include "RequestOutAsync.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace CORBA;
using namespace CORBA::Internal;

namespace CORBA {
namespace Core {

DomainProt::~DomainProt ()
{}

IORequest::_ref_type DomainProt::create_request (unsigned response_flags,
	const IOP::ObjectKey& object_key, const Operation& metadata, ReferenceRemote* ref,
	CallbackRef&& callback)
{
	if (zombie ())
		throw COMM_FAILURE ();

	if (callback) {
		assert (response_flags & IORequest::RESPONSE_EXPECTED); // Checked in ReferenceRemote
		return make_pseudo <RequestOutAsync <RequestOutESIOP> > (response_flags, std::ref (*this),
			std::ref (object_key), std::ref (metadata), ref, std::move (callback));
	} else if (response_flags & IORequest::RESPONSE_EXPECTED) {
		return make_pseudo <RequestOutSync <RequestOutESIOP> > (response_flags, std::ref (*this),
			std::ref (object_key), std::ref (metadata), ref);
	} else {
		return make_pseudo <RequestOutESIOP> (response_flags, std::ref (*this),
			std::ref (object_key), std::ref (metadata), ref);
	}
}

}
}
