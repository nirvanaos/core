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
#include <CORBA/AbstractBase_s.h>
#include <CORBA/Object_s.h>
#include "IDL/Proxy/IOReference_s.h"
#include "offset_ptr.h"

namespace CORBA {
namespace Internal {
namespace Core {

class RequestLocal;

/// \brief Base for servant-side proxies.
class ServantProxyBase :
	public ServantTraits <ServantProxyBase>,
	public LifeCycleDynamic <ServantProxyBase>,
	public Skeleton <ServantProxyBase, Object>,
	public Skeleton <ServantProxyBase, IOReference>,
	public Skeleton <ServantProxyBase, AbstractBase>,
	public ProxyManager
{
	class GarbageCollector;
public:
	typedef ::Nirvana::Core::AtomicCounter <true> RefCnt;

	template <class I>
	static Bridge <I>* _duplicate (Bridge <I>* itf)
	{
		if (itf)
			_implementation (itf)._add_ref_proxy ();
		return itf;
	}

	template <class I>
	static void _release (Bridge <I>* itf)
	{
		if (itf)
			_implementation (itf)._remove_ref_proxy ();
	}

	void _add_ref_proxy ()
	{
		RefCnt::IntegralType cnt = ref_cnt_proxy_.increment ();
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

	virtual RefCnt::IntegralType _remove_ref_proxy () NIRVANA_NOEXCEPT;

	inline
	IORequest::_ref_type create_request (OperationIndex op);

	inline
	IORequest::_ref_type create_oneway (OperationIndex op);

	void invoke (RequestLocal& rq) NIRVANA_NOEXCEPT;

	void send (IORequest::_ref_type& rq, const Nirvana::DeadlineTime& deadline);

	Nirvana::Core::MemContext* mem_context () const NIRVANA_NOEXCEPT
	{
		Nirvana::Core::SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			return &sd->mem_context ();
		return nullptr;
	}

	/// Returns synchronization context for the target object.
	Nirvana::Core::SyncContext& sync_context () const NIRVANA_NOEXCEPT
	{
		return *sync_context_;
	}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_servant_.increment ();
	}

	virtual void _remove_ref () NIRVANA_NOEXCEPT = 0;

	ULong _refcount_value () const NIRVANA_NOEXCEPT
	{
		Nirvana::Core::RefCounter::IntegralType ucnt = ref_cnt_servant_;
		return ucnt > std::numeric_limits <ULong>::max () ? std::numeric_limits <ULong>::max () : (ULong)ucnt;
	}

	void _delete_object ()
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

protected:
	template <class I>
	ServantProxyBase (I_ptr <I> servant,
		const Operation object_ops [3], void* object_impl) :
		ProxyManager (Skeleton <ServantProxyBase, IOReference>::epv_,
			Skeleton <ServantProxyBase, Object>::epv_, 
			Skeleton <ServantProxyBase, AbstractBase>::epv_,
			primary_interface_id (servant),
			object_ops, object_impl),
		ref_cnt_proxy_ (0),
		sync_context_ (&Nirvana::Core::SyncContext::current ())
	{
		size_t offset = offset_ptr ();
		servant_ = offset_ptr (static_cast <Interface::_ptr_type> (servant), offset);
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

	Interface::_ptr_type servant () const NIRVANA_NOEXCEPT
	{
		return servant_;
	}

	virtual void add_ref_1 ();

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual Nirvana::Core::SyncContext& get_sync_context (IOReference::OperationIndex op)
	{
		return *sync_context_;
	}

	template <class GC, class ... Args>
	void run_garbage_collector (Args ... args) const NIRVANA_NOEXCEPT
	{
		try {
			using namespace ::Nirvana::Core;

			ExecDomain& ed = ExecDomain::current ();
			CoreRef <MemContext> mc = push_GC_mem_context (ed);

			try {
				auto gc = CoreRef <Runnable>::create <ImplDynamic <GC> > (std::forward <Args> (args)...);

				Nirvana::DeadlineTime deadline =
					Nirvana::Core::PROXY_GC_DEADLINE == Nirvana::INFINITE_DEADLINE ?
					Nirvana::INFINITE_DEADLINE : Nirvana::Core::Chrono::make_deadline (
						Nirvana::Core::PROXY_GC_DEADLINE);

				// in the current memory context.
				Nirvana::Core::ExecDomain::async_call (deadline, std::move (gc), *sync_context_, mc);
			} catch (...) {
				ed.mem_context_pop ();
				throw;
			}
			ed.mem_context_pop ();

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

	typedef void (*RqProcInternal) (void* servant, IORequest::_ptr_type call);

	static bool call_request_proc (RqProcInternal proc, void* servant, Interface* call);

	template <class Impl, void (*proc) (Impl*, IORequest::_ptr_type)>
	static bool ObjProcWrapper (Interface* servant, Interface* call)
	{
		return call_request_proc ((RqProcInternal)proc, servant, call);
	}

private:
	template <class I>
	static const Char* primary_interface_id (I_ptr <I> servant)
	{
		Interface::_ptr_type primary = servant->_query_interface (0);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> push_GC_mem_context (Nirvana::Core::ExecDomain& ed) const;

protected:
	Nirvana::Core::RefCounter ref_cnt_servant_;

private:
	Interface::_ptr_type servant_;
	RefCnt ref_cnt_proxy_;
	Nirvana::Core::CoreRef <Nirvana::Core::SyncContext> sync_context_;
};

}
}
}

#endif
