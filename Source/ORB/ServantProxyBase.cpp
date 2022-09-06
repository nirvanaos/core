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
#include "../Module.h"

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
		// Unlock module.
		Module* mod = SyncContext::current ().module ();
		if (mod)
			mod->_remove_ref ();
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

ServantProxyBase::~ServantProxyBase ()
{
	assert (&Nirvana::Core::SyncContext::current () == &sync_context ());
}

void ServantProxyBase::add_ref_1 ()
{
	interface_duplicate (&servant_);
}

void ServantProxyBase::_add_ref ()
{
	RefCnt::IntegralType cnt = ref_cnt_proxy_.increment_seq ();
	if (1 == cnt) {
		// Lock module to prevent the binary unloading while there are references
		// to a servant proxy.
		Module* mod = sync_context_->module ();
		if (mod)
			mod->_add_ref ();
		try {
			SYNC_BEGIN (*sync_context_, nullptr);
			add_ref_1 ();
			SYNC_END ();
		} catch (...) {
			if (mod)
				mod->_remove_ref ();
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
	if (0 == cnt) {
		run_garbage_collector <GarbageCollector> (servant_, *sync_context_);
	} else if (cnt < 0) {
		// User violate reference counter rules.
		// TODO: Log error
		ref_cnt_proxy_.increment ();
		cnt = 0;
	}

	return cnt;
}

MemContext* ServantProxyBase::memory () const NIRVANA_NOEXCEPT
{
	MemContext* mc = nullptr;
	SyncDomain* sd = sync_context_->sync_domain ();
	if (sd)
		mc = &sd->mem_context ();
	return mc;
}

IORequest::_ref_type ServantProxyBase::create_request (OperationIndex op, UShort flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	check_create_request (op, flags);

	UShort response_flags = flags & 3;
	MemContext* mem = memory ();
	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			mem, response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			mem, response_flags);
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
