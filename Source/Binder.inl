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
#ifndef NIRVANA_CORE_BINDER_INL_
#define NIRVANA_CORE_BINDER_INL_
#pragma once

#include "Binder.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Executable.h"
#include "ORB/RequestLocalBase.h"
#include <Nirvana/BindErrorUtl.h>
#include <CORBA/Proxy/ProxyBase.h>

namespace Nirvana {
namespace Core {

class Binder::Request : public CORBA::Core::RequestLocalBase
{
public:
	static Ref <Request> create ()
	{
		return Ref <Request>::create <CORBA::Core::RequestLocalImpl <Request> > ();
	}

protected:
	Request () : RequestLocalBase (&BinderMemory::heap (),
		CORBA::Internal::IORequest::RESPONSE_EXPECTED | CORBA::Internal::IORequest::RESPONSE_DATA)
	{}
};

inline
Main::_ptr_type Binder::bind (Executable& mod)
{
	const ModuleStartup* startup = nullptr;

	BindResult ret;
	Ref <Request> rq = Request::create ();
	rq->invoke ();
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	try {
		startup = singleton_->module_bind (mod._get_ptr (), mod.metadata (), nullptr);
		try {
			if (!startup || !startup->startup)
				BindError::throw_message ("Entry point not found");
			singleton_->binary_map_.add (mod);
		} catch (...) {
			release_imports (mod._get_ptr (), mod.metadata ());
			throw;
		}
		rq->success ();
	} catch (CORBA::UserException& ex) {
		rq->set_exception (std::move (ex));
	}
	SYNC_END ();
	CORBA::Internal::ProxyRoot::check_request (rq->_get_ptr ());
	return Main::_check (startup->startup);
}

inline
void Binder::unbind (Executable& mod) noexcept
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	singleton_->binary_map_.remove (mod);
	SYNC_END ();
	release_imports (mod._get_ptr (), mod.metadata ());
}

}
}

#endif
