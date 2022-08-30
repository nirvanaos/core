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
#ifndef NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_
#define NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_
#pragma once

#include <CORBA/Server.h>
#include "../AtomicCounter.h"
#include "../ExecDomain.h"
#include "../Chrono.h"
#include "ProxyManager.h"
#include "offset_ptr.h"

namespace CORBA {
namespace Core {

/// \brief Base for servant-side proxies.
class ServantProxyBase :
	public ProxyManager
{
	class GarbageCollector;
public:
	typedef Nirvana::Core::AtomicCounter <true> RefCnt;

	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, UShort flags) override;

	/// Returns synchronization context for the target object.
	Nirvana::Core::SyncContext& sync_context () const NIRVANA_NOEXCEPT
	{
		return *sync_context_;
	}

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual Nirvana::Core::SyncContext& get_sync_context (Internal::IOReference::OperationIndex op)
		const NIRVANA_NOEXCEPT override
	{
		if (op.interface_idx () == 0 && op.operation_idx () != (UShort)ObjectOp::NON_EXISTENT)
			return ProxyManager::get_sync_context (op);
		return *sync_context_;
	}

protected:
	template <class I>
	ServantProxyBase (Internal::I_ptr <I> servant) :
		ProxyManager (primary_interface_id (servant)),
		ref_cnt_proxy_ (0),
		sync_context_ (&Nirvana::Core::SyncContext::current ())
	{
		size_t offset = offset_ptr ();
		servant_ = offset_ptr (static_cast <Internal::Interface::_ptr_type> (servant), offset);
		// Fill implementation pointers
		for (InterfaceEntry* ie = interfaces ().begin (); ie != interfaces ().end (); ++ie) {
			if (!ie->implementation) {
				Internal::Interface::_ptr_type impl = servant->_query_interface (ie->iid);
				if (!impl)
					throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
				ie->implementation = offset_ptr (impl, offset);
			}
		}
	}

	Internal::Interface::_ptr_type servant () const NIRVANA_NOEXCEPT
	{
		return servant_;
	}

	virtual void add_ref_1 ();
	virtual void _add_ref () override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	template <class> friend class Nirvana::Core::CoreRef;

	RefCnt::IntegralType _remove_ref_proxy () NIRVANA_NOEXCEPT;

	template <class GC, class Arg>
	void run_garbage_collector (Arg arg, Nirvana::Core::SyncContext& sync_context) const NIRVANA_NOEXCEPT
	{
		try {
			using namespace Nirvana::Core;

			ExecDomain& ed = ExecDomain::current ();
			CoreRef <MemContext> mc = push_GC_mem_context (ed, sync_context);

			try {
				auto gc = CoreRef <Runnable>::create <ImplDynamic <GC> > (arg);

				Nirvana::DeadlineTime deadline =
					Nirvana::Core::PROXY_GC_DEADLINE == Nirvana::INFINITE_DEADLINE ?
					Nirvana::INFINITE_DEADLINE : Nirvana::Core::Chrono::make_deadline (
						Nirvana::Core::PROXY_GC_DEADLINE);

				// in the current memory context.
				ExecDomain::async_call (deadline, std::move (gc), sync_context, mc);
			} catch (...) {
				ed.mem_context_pop ();
				throw;
			}
			ed.mem_context_pop ();

		} catch (...) {
			// Async call failed, maybe resources are exausted.
			// Fallback to collect garbage in the current ED.
			try {
				SYNC_BEGIN (sync_context, nullptr)
				Nirvana::Core::ImplStatic <GC> (arg).run ();
				SYNC_END ()
			} catch (...) {
				// Swallow exceptions.
				// TODO: Log error.
			}
		}
	}

private:
	template <class I>
	static const Char* primary_interface_id (Internal::I_ptr <I> servant)
	{
		Internal::Interface::_ptr_type primary = servant->_query_interface (0);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> push_GC_mem_context (
		Nirvana::Core::ExecDomain& ed, Nirvana::Core::SyncContext& sc) const;

private:
	Internal::Interface::_ptr_type servant_;
	RefCnt ref_cnt_proxy_;
	Nirvana::Core::CoreRef <Nirvana::Core::SyncContext> sync_context_;
};

}
}

#endif
