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
#include "ReferenceLocal.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestLocalPOA::RequestLocalPOA (ReferenceLocal& reference, IOReference::OperationIndex op,
	unsigned response_flags) :
	RequestLocalBase (nullptr, response_flags),
	reference_ (&reference),
	op_idx_ (op)
{
	callee_memory_ = caller_memory_;
}

void RequestLocalPOA::_add_ref () noexcept
{
	RequestLocalBase::_add_ref ();
}

MemContext* RequestLocalPOA::memory () const noexcept
{
	return caller_memory_;
}

const IOP::ObjectKey& RequestLocalPOA::object_key () const noexcept
{
	return reference_->object_key ();
}

CORBA::Internal::StringView <Char> RequestLocalPOA::operation () const noexcept
{
	return reference_->operation_metadata (op_idx ()).name;
}

void RequestLocalPOA::set_exception (Any& e)
{
	RequestLocalBase::set_exception (e);
}

void RequestLocalPOA::serve (const ServantProxyBase& proxy)
{
	SyncContext& sc = proxy.get_sync_context (op_idx ());
	MemContext* mc = nullptr;
	SyncDomain* sd = sc.sync_domain ();
	if (sd)
		mc = &sd->mem_context ();
	else
		mc = memory ();
	SYNC_BEGIN (sc, mc);
	proxy.invoke (op_idx (), _get_ptr ());
	SYNC_END ();
}

void RequestLocalPOA::invoke ()
{
	RequestLocalBase::invoke (); // rewind etc.
	PortableServer::Core::POA_Root::invoke (servant_reference <RequestInPOA> (this), false);
}

bool RequestLocalPOA::is_cancelled () const noexcept
{
	return RequestLocalBase::is_cancelled ();
}

void RequestLocalOnewayPOA::invoke ()
{
	RequestLocalBase::invoke (); // rewind etc.
	PortableServer::Core::POA_Root::invoke_async (servant_reference <RequestInPOA> (this),
		ExecDomain::current ().get_request_deadline (!response_flags ()));
}

void RequestLocalOnewayPOA::serve (const ServantProxyBase& proxy)
{
	if (RequestLocalOnewayPOA::is_cancelled ())
		return;
	RequestLocalPOA::serve (proxy);
}

void RequestLocalAsyncPOA::cancel () noexcept
{
	if (set_cancelled ())
		response_flags_ = 0; // Prevent marshaling
}

void RequestLocalAsyncPOA::finalize () noexcept
{
	Base::finalize ();
	RqHelper::call_completed (callback_, _get_ptr ());
}

}
}
