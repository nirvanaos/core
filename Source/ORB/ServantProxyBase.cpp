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

void ServantProxyBase::invoke (OperationIndex op, Internal::IORequest::_ptr_type rq) NIRVANA_NOEXCEPT
{
	try {
		try {
			size_t itf_idx = op.interface_idx ();
			size_t op_idx = op.operation_idx ();
			if (0 == itf_idx) { // Object operation
				serve_object_request ((ObjectOp)op_idx, rq);
			} else {
				--itf_idx;
				assert (itf_idx < interfaces ().size ());
				const InterfaceEntry& ie = interfaces () [itf_idx];
				assert (op_idx < ie.operations.size);
				RequestProc invoke = ie.operations.p [op_idx].invoke;
				if (!(*invoke) (&ie.implementation, &rq))
					throw UNKNOWN ();
			}
		} catch (Exception& e) {
			Any any;
			any <<= std::move (e);
			rq->set_exception (any);
		}
	} catch (...) {
		Any any;
		try {
			any <<= UNKNOWN ();
			rq->set_exception (any);
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
