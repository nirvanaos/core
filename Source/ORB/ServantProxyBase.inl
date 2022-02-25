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
#ifndef NIRVANA_ORB_CORE_SERVANTPROXYBASE_INL_
#define NIRVANA_ORB_CORE_SERVANTPROXYBASE_INL_
#pragma once

#include "RequestLocal.h"

namespace CORBA {
namespace Internal {
namespace Core {

inline
IORequest::_ref_type ServantProxyBase::create_request (OperationIndex op)
{
	return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
		mem_context ());
}

inline
IORequest::_ref_type ServantProxyBase::create_oneway (OperationIndex op)
{
	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> target_memory = mem_context ();
	if (!target_memory)
		target_memory = Nirvana::Core::MemContextUser::create ();
	Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
	ed.mem_context_push (target_memory);
	IORequest::_ref_type ret;
	try {
		ret = make_pseudo <RequestLocalImpl <RequestLocalOneway> > (std::ref (*this),
			op, target_memory);
	} catch (...) {
		ed.mem_context_pop ();
		throw;
	}
	ed.mem_context_pop ();
	return ret;
}

inline
void ServantProxyBase::send (IORequest::_ref_type& rq,
	const Nirvana::DeadlineTime& deadline)
{
	if (!rq)
		throw BAD_PARAM ();
	RequestLocal* prq = static_cast <RequestLocal*> (&(IORequest::_ptr_type)rq);
	switch (prq->kind ()) {
		case RqKind::SYNC:
			Nirvana::throw_BAD_PARAM ();
			break;

		case RqKind::ONEWAY:
		{
			Nirvana::Core::CoreRef <Nirvana::Core::Runnable> ref =
				&static_cast <RequestLocalOneway&> (*prq);
			rq = nullptr;
			Nirvana::Core::ExecDomain::async_call (deadline, std::move (ref), sync_context (), prq->memory ());
		} break;

		default:
			Nirvana::Core::ExecDomain::async_call (deadline,
				&static_cast <RequestLocalAsync&> (*prq), sync_context (), prq->memory ());
	}
}

}
}
}

#endif
