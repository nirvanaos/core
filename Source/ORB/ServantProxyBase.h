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

#include "../AtomicCounter.h"
#include "../Synchronized.h"
#include "../ExecDomain.h"
#include "../Runnable.h"
#include "ProxyManager.h"
#include <CORBA/AbstractBase_s.h>
#include <CORBA/Object_s.h>
#include <CORBA/ImplementationPseudo.h>
#include <CORBA/Proxy/IOReference_s.h>
#include <CORBA/Proxy/IORequest_s.h>
#include "LifeCycleStack.h"
#include "ServantMarshaler.h"
#include <utility>

namespace CORBA {
namespace Nirvana {
namespace Core {

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
				SYNC_BEGIN (*sync_context_);
				add_ref_1 ();
				SYNC_END ();
			} catch (...) {
				ref_cnt_.decrement ();
				throw;
			}
		}
	}

	virtual RefCnt::IntegralType _remove_ref ();

protected:
	ServantProxyBase (AbstractBase::_ptr_type servant, const Operation object_ops [3], void* object_impl, ::Nirvana::Core::SyncContext& sync_context);

	virtual void add_ref_1 ();

	/// Returns synchronization context for the target object.
	::Nirvana::Core::SyncContext& sync_context () const
	{
		return *sync_context_;
	}

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual ::Nirvana::Core::SyncContext& get_sync_context (OperationIndex op)
	{
		return *sync_context_;
	}

	template <class GC, class ... Args>
	void run_garbage_collector (Args ... args) const NIRVANA_NOEXCEPT
	{
		try {
			using namespace ::Nirvana::Core;
			auto gc = Core_var <Runnable>::create <ImplDynamic <GC> > (std::forward <Args> (args)...);
			// TODO: Garbage collector deadline must not be infinite, but reasonable enough.
			::Nirvana::Core::ExecDomain::async_call (::Nirvana::INFINITE_DEADLINE, *gc, sync_context_->sync_domain ());
		} catch (...) {
			// Async call failed, maybe resources are exausted or shutdown was initiated.
			// Fallback to collect garbage in current thread.
			try {
				::Nirvana::Core::ImplStatic <GC> (std::forward <Args> (args)...).run ();
			} catch (...) {
				// Swallow exceptions.
				// TODO: Log error.
			}
		}
	}

	template <class I, void (*proc) (I*, IORequest::_ptr_type, ::Nirvana::ConstPointer, Unmarshal::_ref_type&, ::Nirvana::Pointer)>
	static void ObjProcWrapper (Interface* servant, Interface* call,
		::Nirvana::ConstPointer in_params,
		Interface** unmarshaler,
		::Nirvana::Pointer out_params)
	{
		try {
			IORequest::_ptr_type rq = IORequest::_check (call);
			try {
				proc ((I*)(void*)servant, rq, in_params, Type <Unmarshal>::inout (unmarshaler), out_params);
				rq->success ();
			} catch (Exception& e) {
				Any any;
				any <<= std::move (e);
				rq->set_exception (any);
			}
		} catch (...) {
		}
	}

	class Request :
		public ImplementationPseudo <Request, IORequest>,
		public LifeCycleStack
	{
	public:
		Request () :
			sync_context_ (&::Nirvana::Core::SyncContext::current ()),
			success_ (false)
		{
			exception_.reset ();
		}

		Marshal_ptr marshaler ()
		{
			if (!marshaler_)
				marshaler_ = (new ServantMarshaler (*sync_context_))->marshaler ();
			return marshaler_;
		}

		void set_exception (Any& exc)
		{
			I_ptr <TypeCode> tc = exc.type ();
			if (tc)
				Type <Any>::marshal_out (exc, marshaler (), exception_);
		}

		void success ()
		{
			success_ = true;
		}

		Unmarshal::_ref_type check ()
		{
			Unmarshal::_ref_type u = ServantMarshaler::unmarshaler (marshaler_);
			marshaler_ = nullptr;
			if (!success_) {
				if (exception_.type ()) {
					Any exc;
					Type <Any>::unmarshal (exception_, u, exc);
					Octet tmp [sizeof (SystemException)];
					SystemException* pse = (SystemException*)tmp;
					if (exc >>= *pse)
						pse->_raise ();
					else
						throw UnknownUserException (std::move (exc));
				} else
					throw UNKNOWN ();
			}
			return u;
		}

	private:
		::Nirvana::Core::Core_var <::Nirvana::Core::SyncContext> sync_context_;
		Marshal::_ref_type marshaler_;
		ABI <Any> exception_;
		bool success_;
	};

public:
	Marshal::_ref_type create_marshaler () const
	{
		return make_reference <ServantMarshaler> (std::ref (*sync_context_))->marshaler ();
	}

	Unmarshal::_ref_type call (OperationIndex op,
		const void* in_params, size_t in_params_size,
		Marshal::_ref_type& marshaler,
		void* out_params, size_t out_params_size)
	{
		size_t idx = interface_idx (op);
		if (idx >= interfaces ().size ())
			throw BAD_OPERATION ();
		const InterfaceEntry& ie = interfaces () [idx];
		idx = operation_idx (op);
		if (idx >= ie.operations.size)
			throw BAD_OPERATION ();
		Unmarshal::_ref_type u = ServantMarshaler::unmarshaler (marshaler);
		marshaler = nullptr;
		Request request;
		SYNC_BEGIN (get_sync_context (op));
		(ie.operations.p [idx].invoke) (&ie.implementation, &request._get_ptr (), in_params, &Type <Unmarshal>::C_inout (u), out_params);
		SYNC_END ();
		return request.check ();
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
	::Nirvana::Core::Core_var <::Nirvana::Core::SyncContext> sync_context_;
};

}
}
}

#endif
