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

#include "../AtomicCounter.h"
#include "../Synchronized.h"
#include "../ExecDomain.h"
#include "../Runnable.h"
#include "../Chrono.h"
#include "ProxyManager.h"
#include <CORBA/AbstractBase_s.h>
#include <CORBA/Object_s.h>
#include <CORBA/ImplementationPseudo.h>
#include "IDL/IOReference_s.h"
#include "IDL/IORequest_s.h"
#include "LifeCycleStack.h"
#include <utility>

namespace CORBA {
namespace Internal {
namespace Core {

class RequestLocal;

/// \brief Base for servant-side proxies.
class ServantProxyBase :
	public ServantTraits <ServantProxyBase>,
	public LifeCycleRefCnt <ServantProxyBase>,
	public InterfaceImplBase <ServantProxyBase, AbstractBase>,
	public Skeleton <ServantProxyBase, Object>,
	public Skeleton <ServantProxyBase, IOReference>,
	public ProxyManager
{
	class GarbageCollector;
public:
	typedef ::Nirvana::Core::AtomicCounter <false> RefCnt;

	void _add_ref ()
	{
		RefCnt::IntegralType cnt = ref_cnt_.increment ();
		if (1 == cnt) {
			try {
				SYNC_BEGIN (*sync_context_, nullptr);
				add_ref_1 ();
				SYNC_END ();
			} catch (...) {
				ref_cnt_.decrement ();
				throw;
			}
		}
	}

	virtual RefCnt::IntegralType _remove_ref () NIRVANA_NOEXCEPT;

	IORequest::_ref_type create_request ();
	IORequest::_ref_type create_request_oneway ();

	void call (IORequest::OperationIndex op, RequestLocal& rq);

	Nirvana::Core::MemContext* mem_context () const
	{
		Nirvana::Core::SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			return &sd->mem_context ();
		return nullptr;
	}

	/// Returns synchronization context for the target object.
	Nirvana::Core::SyncContext& sync_context () const
	{
		return *sync_context_;
	}

protected:
	ServantProxyBase (AbstractBase::_ptr_type servant, const Operation object_ops [3], void* object_impl);

	virtual void add_ref_1 ();

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual Nirvana::Core::SyncContext& get_sync_context (IORequest::OperationIndex op)
	{
		return *sync_context_;
	}

	template <class GC, class ... Args>
	void run_garbage_collector (Args ... args) const NIRVANA_NOEXCEPT
	{
		try {
			using namespace ::Nirvana::Core;
			auto gc = CoreRef <Runnable>::create <ImplDynamic <GC> > (std::forward <Args> (args)...);
			Nirvana::DeadlineTime deadline = 
				Nirvana::Core::PROXY_GC_DEADLINE == Nirvana::INFINITE_DEADLINE ?
				Nirvana::INFINITE_DEADLINE : Nirvana::Core::Chrono::make_deadline (Nirvana::Core::PROXY_GC_DEADLINE);
			Nirvana::Core::ExecDomain::async_call (deadline, *gc, *sync_context_);
		} catch (...) {
			// Async call failed, maybe resources are exausted.
			// Fallback to collect garbage in the current thread.
			try {
				SYNC_BEGIN (*sync_context_, nullptr)
				::Nirvana::Core::ImplStatic <GC> (std::forward <Args> (args)...).run ();
				SYNC_END ()
			} catch (...) {
				// Swallow exceptions.
				// TODO: Log error.
			}
		}
	}

	template <class I, void (*proc) (I*, IORequest::_ptr_type)>
	static bool ObjProcWrapper (Interface* servant, Interface* call)
	{
		return call_request_proc ((RqProcInternal)proc, servant, call);
	}

private:
	static const Char* primary_interface_id (AbstractBase::_ptr_type servant)
	{
		Interface::_ptr_type primary = servant->_query_interface (0);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

private:
	AbstractBase::_ptr_type servant_;
	RefCnt ref_cnt_;
	Nirvana::Core::CoreRef <Nirvana::Core::SyncContext> sync_context_;
};

}
}
}

#endif
