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
#include "ProxyObjectImplicit.h"
#include "POA_Base.h"

namespace CORBA {
namespace Core {

// Called in the servant synchronization context.
// Note that sync context may be out of synchronization domain
// for the stateless objects.
void ProxyObjectImplicit::add_ref_1 ()
{
	Base::add_ref_1 ();

	if (!change_state (DEACTIVATION_SCHEDULED, DEACTIVATION_CANCELLED)) {
		if (change_state (INACTIVE, IMPLICIT_ACTIVATION)) {
			try {
				adapter_->activate_object (servant ());
			} catch (...) {
				if (change_state (IMPLICIT_ACTIVATION, INACTIVE))
					throw;
			}
		}
	}
}

void ProxyObjectImplicit::activate (PortableServer::Core::ObjectKeyRef&& key) NIRVANA_NOEXCEPT
{
	Base::activate (std::move (key));
	if (!change_state (IMPLICIT_ACTIVATION, ACTIVE))
		change_state (INACTIVE, ACTIVE_EXPLICIT);
}

}
}
