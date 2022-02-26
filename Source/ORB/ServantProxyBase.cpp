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
#include "ServantProxyBase.inl"
#include "offset_ptr.h"

namespace CORBA {
namespace Internal {
namespace Core {

using namespace ::Nirvana::Core;

class ServantProxyBase::GarbageCollector :
	public UserObject,
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
	ref_cnt_ (0),
	sync_context_ (&SyncContext::current ())
{
	size_t offset = offset_ptr ();
	servant_ = offset_ptr (servant, offset);
	// Fill implementation pointers
	for (InterfaceEntry* ie = interfaces ().begin (); ie != interfaces ().end (); ++ie) {
		if (!ie->implementation) {
			Interface::_ptr_type impl = servant->_query_interface (ie->iid);
			if (!impl)
				throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
			ie->implementation = offset_ptr (impl, offset);
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

CoreRef <MemContext> ServantProxyBase::push_GC_mem_context (Nirvana::Core::ExecDomain& ed) const
{
	CoreRef <MemContext> mc = mem_context ();
	if (!mc) {
		mc = ed.mem_context_ptr ();
		if (!mc)
			mc = &g_shared_mem_context;
	}
	ed.mem_context_push (mc);
	return mc;
}

void ServantProxyBase::invoke (RequestLocal& rq) NIRVANA_NOEXCEPT
{
	try {
		OperationIndex op = rq.op_idx ();
		size_t idx = op.interface_idx ();
		if (idx >= interfaces ().size ())
			throw BAD_OPERATION ();
		const InterfaceEntry& ie = interfaces () [idx];
		idx = op.operation_idx ();
		if (idx >= ie.operations.size)
			throw BAD_OPERATION ();
#ifdef _DEBUG
		size_t dbg_stack_size0 = ExecDomain::current ().dbg_context_stack_size_;
#endif
		bool success;
		SYNC_BEGIN (get_sync_context (op), rq.memory ());
		success = (ie.operations.p [idx].invoke) (&ie.implementation, &rq);
		SYNC_END ();
#ifdef _DEBUG
		size_t dbg_stack_size1 = ExecDomain::current ().dbg_context_stack_size_;
		assert (dbg_stack_size0 == dbg_stack_size1);
#endif
		if (!success)
			throw UNKNOWN ();
	} catch (Exception& e) {
		Any any;
		try {
			any <<= std::move (e);
			rq.set_exception (any);
		} catch (...) {
		}
	}
}

bool ServantProxyBase::call_request_proc (RqProcInternal proc, void* servant, Interface* call)
{
	IORequest::_ptr_type rq = IORequest::_nil ();
	try {
		rq = IORequest::_check (call);
		proc (servant, rq);
		rq->success ();
	} catch (Exception& e) {
		if (!rq)
			return false;

		Any any;
		any <<= std::move (e);
		rq->set_exception (any);
	}
	return true;
}

}
}
}
