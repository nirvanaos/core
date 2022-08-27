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
#include "POAManager.h"

using namespace CORBA;

namespace PortableServer {
namespace Core {

void POAManager::ServeRequest::run ()
{
	try {
		const CORBA::Core::ProxyLocal* proxy = CORBA::Core::ProxyLocal::get_proxy (adapter->_this ());
		SYNC_BEGIN (proxy->sync_context (), nullptr);
		if (!request->is_cancelled ()) {
			// Check that adapter is not destroyed.
			if (adapter->is_destroyed ())
				throw POA::AdapterNonExistent ();
			adapter->serve (*request);
		}
		SYNC_END ();
	} catch (Exception& e) {
		Any a;
		a <<= std::move (e);
		try {
			request->set_exception (a);
		} catch (...) {}
	} catch (...) {
		Any a;
		a <<= UNKNOWN ();
		try {
			request->set_exception (a);
		} catch (...) {}
	}
}

void POAManager::ServeRequest::on_crash (const siginfo& signal) NIRVANA_NOEXCEPT
{
	Any ex = CORBA::Core::RqHelper::signal2exception (signal);
	request->set_exception (ex);
}

void POAManager::discard_queued_requests ()
{
	while (!queue_.empty ()) {
		if (!queue_.top ().request->is_cancelled ()) {
			CORBA::Any a;
			a <<= CORBA::TRANSIENT (MAKE_OMG_MINOR (1));
			queue_.top ().request->set_exception (a);
		}
		queue_.pop ();
	}
}

}
}
