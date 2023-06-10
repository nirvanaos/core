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
#ifndef NIRVANA_ORB_CORE_REQUESTEVENT_H_
#define NIRVANA_ORB_CORE_REQUESTEVENT_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/Proxy/IOReference_s.h>
#include "../Event.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE RequestEvent :
	public Nirvana::Core::Event,
	public CORBA::servant_traits <Internal::RequestCallback>::Servant <RequestEvent>,
	public CORBA::Internal::LifeCycleRefCnt <RequestEvent>
{
	DECLARE_CORE_INTERFACE

public:
	void completed (Internal::IORequest::_ptr_type request) noexcept
	{
		signal ();
	}

protected:
	RequestEvent ();
	~RequestEvent ();
};

}
}

#endif
