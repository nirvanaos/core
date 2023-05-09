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

class ServantProxyBase::GC :
	public Runnable
{
public:
	GC (Interface::_ptr_type servant) :
		servant_ (servant)
	{}

private:
	virtual void run ();

private:
	Interface::_ptr_type servant_;
};

void ServantProxyBase::GC::run ()
{
	collect_garbage (servant_);
}

ServantProxyBase::~ServantProxyBase ()
{
	assert (&Nirvana::Core::SyncContext::current () == &sync_context ());
}

void ServantProxyBase::_remove_ref () NIRVANA_NOEXCEPT
{
	RefCntProxy::IntegralType cnt = ref_cnt_.decrement_seq ();
	if (0 == cnt) {
		if (servant_)
			run_garbage_collector ();
		else
			delete this;
	}
}

void ServantProxyBase::delete_proxy () NIRVANA_NOEXCEPT
{
	// Called in the servant sync context
	assert (&Nirvana::Core::SyncContext::current () == sync_context_);
	if (!ref_cnt_.load ())
		delete this;
	else {
		assert (false);
		reset_servant ();
	}
}

void ServantProxyBase::reset_servant () NIRVANA_NOEXCEPT
{
	servant_ = nullptr;
	ProxyManager::reset_servant ();
}

void ServantProxyBase::add_ref_servant () const
{
	// Called in the servant synchronization context on getting the first reference to proxy.
	if (&SyncContext::current () == sync_context_)
		interface_duplicate (&servant_);
	else {
		SYNC_BEGIN (*sync_context_, nullptr)
			interface_duplicate (&servant_);
		SYNC_END ()
	}

	if (servant_) {
		// Lock module to prevent unloading the binary while there are references
		// to a servant proxy.
		Module* mod = sync_context_->module ();
		if (mod)
			mod->_add_ref ();
	}
}

void ServantProxyBase::collect_garbage (Internal::Interface::_ptr_type servant) NIRVANA_NOEXCEPT
{
	// Called in the servant synchronization context on releasing the last reference to proxy.
	interface_release (&servant);
	Module* mod = SyncContext::current ().module ();
	if (mod)
		mod->_remove_ref ();
}

void ServantProxyBase::run_garbage_collector () const NIRVANA_NOEXCEPT
{
	using namespace Nirvana::Core;

	ExecDomain& ed = ExecDomain::current ();
	if (ed.restricted_mode () != ExecDomain::RestrictedMode::SUPPRESS_ASYNC_GC) {
		try {
			Nirvana::DeadlineTime deadline =
				Nirvana::Core::PROXY_GC_DEADLINE == Nirvana::INFINITE_DEADLINE ?
				Nirvana::INFINITE_DEADLINE : Nirvana::Core::Chrono::make_deadline (
					Nirvana::Core::PROXY_GC_DEADLINE);

			// in the current memory context.
			ExecDomain::async_call <GC> (deadline, sync_context (), nullptr, servant_);
			return;
		} catch (...) {
			// Async call failed, maybe resources are exausted.
			// Fallback to collect garbage synchronous.
		}
	}
	if (&sync_context () != &SyncContext::current ()) {
		try {
			SYNC_BEGIN (sync_context (), nullptr)
				collect_garbage (servant_);
			SYNC_END ()
		} catch (...) {
				assert (false);
				// Swallow exceptions.
			}
	} else
		collect_garbage (servant_);
}

MemContext* ServantProxyBase::memory () const NIRVANA_NOEXCEPT
{
	MemContext* mc = nullptr;
	SyncDomain* sd = sync_context_->sync_domain ();
	if (sd)
		mc = &sd->mem_context ();
	return mc;
}

IORequest::_ref_type ServantProxyBase::create_request (OperationIndex op, unsigned flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	check_create_request (op, flags);

	unsigned response_flags = flags & 3;
	MemContext* mem = memory ();
	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			mem, response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			mem, response_flags);
}

}
}
