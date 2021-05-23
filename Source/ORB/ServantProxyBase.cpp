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
#include "ServantProxyBase.h"
#include "../Runnable.h"

namespace CORBA {
namespace Internal {
namespace Core {

using namespace ::Nirvana::Core;

class ServantProxyBase::GarbageCollector :
	public CoreObject,
	public Runnable
{
public:
	GarbageCollector (Interface::_ptr_type servant) :
		servant_ (servant)
	{}

	~GarbageCollector ()
	{}

	void run ()
	{
		interface_release (&servant_);
	}

private:
	Interface::_ptr_type servant_;
};

ServantProxyBase::ServantProxyBase (AbstractBase::_ptr_type servant, 
	const Operation object_ops [3], void* object_impl) :
	ProxyManager (Skeleton <ServantProxyBase, IOReference>::epv_, 
		Skeleton <ServantProxyBase, Object>::epv_, primary_interface_id (servant), 
		object_ops, object_impl),
	servant_ (servant),
	ref_cnt_ (0),
	sync_context_ (&SyncContext::current ())
{
	// Fill implementation pointers
	for (InterfaceEntry* ie = interfaces ().begin (); ie != interfaces ().end (); ++ie) {
		if (!ie->implementation) {
			if (!(ie->implementation = servant->_query_interface (ie->iid)))
				throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
		}
	}
}

void ServantProxyBase::add_ref_1 ()
{
	interface_duplicate (&servant_);
}

ServantProxyBase::RefCnt::IntegralType ServantProxyBase::_remove_ref () NIRVANA_NOEXCEPT
{
	RefCnt::IntegralType cnt = ref_cnt_.decrement ();
	if (!cnt) {
		run_garbage_collector <GarbageCollector> (servant_);
	} else if (cnt < 0) {
		// TODO: Log error
		ref_cnt_.increment ();
		cnt = 0;
	}
		
	return cnt;
}

}
}
}
