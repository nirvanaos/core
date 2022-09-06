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
#include "RequestLocalPOA.h"
#include "POA_Root.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

void RequestLocalPOA::serve_request (ProxyObject& proxy, Internal::IOReference::OperationIndex op,
	MemContext* memory)
{
	SyncContext& sc = proxy.get_sync_context (op);
	if (sc.sync_domain ())
		memory = nullptr;
	SYNC_BEGIN (sc, memory);
	proxy.invoke (op, _get_ptr ());
	SYNC_END ();
}

void RequestLocalPOA::invoke ()
{
	RequestLocalBase::invoke (); // rewind etc.
	PortableServer::Core::POA_Root::invoke (CoreRef <RequestInPOA> (this), false);
}

void RequestLocalAsyncPOA::invoke ()
{
	RequestLocalBase::invoke (); // rewind etc.
	PortableServer::Core::POA_Root::invoke_async (CoreRef <RequestInPOA> (this),
		ExecDomain::current ().get_request_deadline (!response_flags ()));
}

void RequestLocalAsyncPOA::serve_request (ProxyObject& proxy, Internal::IOReference::OperationIndex op,
	Nirvana::Core::MemContext* memory)
{
	if (response_flags ()) {
		exec_domain_ = &ExecDomain::current ();
		if (RequestLocalAsyncPOA::is_cancelled ())
			return;
	}
	RequestLocalPOA::serve_request (proxy, op, memory);
}

void RequestLocalAsyncPOA::cancel () NIRVANA_NOEXCEPT
{
	if (set_cancelled ()) {
		assert (exec_domain_);
		try {
			exec_domain_->abort ();
		} catch (...) {}
	}
}

}
}
