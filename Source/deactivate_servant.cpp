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
#include "deactivate_servant.h"
#include "ORB/POA_Root.h"

namespace Nirvana {
namespace Core {

void deactivate_servant (PortableServer::Servant servant) noexcept
{
	assert (servant);
	if (!PortableServer::Core::POA_Root::shutdown_started ()) {
		try {
			PortableServer::POA::_ref_type adapter = servant->_default_POA ();
			// Core returns nil reference if shutdown is started
			if (adapter)
				adapter->deactivate_object (adapter->servant_to_id (servant));
		} catch (...) {
			assert (false);
		}
	}
}

}
}
