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

void ServantProxyBase::add_ref_1 ()
{
	interface_duplicate (&servant_);
}

void ServantProxyBase::_add_ref ()
{
	RefCntProxy::IntegralType cnt = ref_cnt_.increment_seq ();
	if (1 == cnt) {
		// Lock module to prevent the binary unloading while there are references
		// to a servant proxy.
		Module* mod = sync_context_->module ();
		if (mod)
			mod->_add_ref ();

		// Call add_ref_1 () in the servant sync context.
		try {
			if (&SyncContext::current () != sync_context_) {
				SYNC_BEGIN (*sync_context_, nullptr);
				add_ref_1 ();
				SYNC_END ();
			} else
				add_ref_1 ();
		} catch (...) {
			if (mod)
				mod->_remove_ref ();
			ref_cnt_.decrement ();
			throw;
		}
	}
}

void ServantProxyBase::_remove_ref () NIRVANA_NOEXCEPT
{
	remove_ref_proxy ();
}

inline
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
			// Fallback to collect garbage in the current ED.
		}
	} else if (&sync_context () != &SyncContext::current ()) {
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

void ServantProxyBase::collect_garbage (Internal::Interface::_ptr_type servant) NIRVANA_NOEXCEPT
{
	interface_release (&servant);
	Module* mod = SyncContext::current ().module ();
	if (mod)
		mod->_remove_ref ();
}

RefCntProxy::IntegralType ServantProxyBase::remove_ref_proxy () NIRVANA_NOEXCEPT
{
	RefCntProxy::IntegralType cnt = ref_cnt_.decrement_seq ();
	if (0 == cnt)
		run_garbage_collector ();

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
