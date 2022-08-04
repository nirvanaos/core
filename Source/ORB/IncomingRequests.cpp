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
#include "IncomingRequests.h"
//#include "RequestIn.h"
#include <algorithm>

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <IncomingRequests::RequestMap> IncomingRequests::request_map_;

class NIRVANA_NOVTABLE IncomingRequests::Process : public Runnable
{
protected:
	Process (const Request& rq) :
		request_ (rq)
	{}

	virtual void run ();

private:
	Request request_;
};

void IncomingRequests::receive (const ClientAddr& source, const Request& rq)
{
	assert (rq.GIOP_version.major () == 1 && rq.GIOP_version.minor () <= 3);

	// Initially schedule async_call with zero deadline because we don't know the deadline yet.
	ExecDomain::async_call (0, CoreRef <Runnable>::create <ImplDynamic <Process> > (ref (rq)),
		g_core_free_sync_context);
}

void IncomingRequests::cancel (const ClientAddr& source, uint32_t request_id) NIRVANA_NOEXCEPT
{
	// TODO: Implement
}

void IncomingRequests::Process::run ()
{

}

}
}
