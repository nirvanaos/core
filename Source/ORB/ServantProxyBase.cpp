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
#include "RequestLocal.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

class NIRVANA_NOVTABLE ServantProxyBase::GarbageCollector :
	public UserObject,
	public Runnable
{
public:
	void run ()
	{
		interface_release (&servant_);
	}

protected:
	GarbageCollector (Interface::_ptr_type servant) :
		servant_ (servant)
	{}

	~GarbageCollector ()
	{}

private:
	Interface::_ptr_type servant_;
};

void ServantProxyBase::add_ref_1 ()
{
	interface_duplicate (&servant_);
}

void ServantProxyBase::_add_ref ()
{
	RefCnt::IntegralType cnt = ref_cnt_proxy_.increment_seq ();
	if (1 == cnt) {
		try {
			SYNC_BEGIN (*sync_context_, nullptr);
			add_ref_1 ();
			SYNC_END ();
		} catch (...) {
			ref_cnt_proxy_.decrement ();
			throw;
		}
	}
}

void ServantProxyBase::_remove_ref () NIRVANA_NOEXCEPT
{
	_remove_ref_proxy ();
}

ServantProxyBase::RefCnt::IntegralType ServantProxyBase::_remove_ref_proxy () NIRVANA_NOEXCEPT
{
	RefCnt::IntegralType cnt = ref_cnt_proxy_.decrement_seq ();
	if (!cnt) {
		run_garbage_collector <GarbageCollector> (servant_, *sync_context_);
	} else if (cnt < 0) {
		// TODO: Log error
		ref_cnt_proxy_.increment ();
		cnt = 0;
	}
		
	return cnt;
}

IORequest::_ref_type ServantProxyBase::create_request (OperationIndex op, UShort flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	UShort response_flags = flags & 3;
	if (response_flags == 2)
		throw INV_FLAG ();

	MemContext* memory = nullptr;
	SyncDomain* sd = sync_context_->sync_domain ();
	if (sd)
		memory = &sd->mem_context ();

	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			memory, response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			memory, response_flags);
}

CoreRef <MemContext> ServantProxyBase::push_GC_mem_context (ExecDomain& ed,
	SyncContext& sc) const
{
	CoreRef <MemContext> mc;
	SyncDomain* sd = sc.sync_domain ();
	if (sd)
		mc = &sd->mem_context ();
	else {
		mc = ed.mem_context_ptr ();
		if (!mc)
			mc = &g_shared_mem_context;
	}
	ed.mem_context_push (mc);
	return mc;
}

}
}
