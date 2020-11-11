/// \file ServantProxyBase.h
#ifndef NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_
#define NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_

#include "../AtomicCounter.h"
#include "../Synchronized.h"
#include "../ExecDomain.h"
#include "ProxyManager.h"
#include <CORBA/AbstractBase_s.h>
#include <CORBA/Object_s.h>
#include <CORBA/ImplementationPseudo.h>
#include <CORBA/Proxy/IOReference_s.h>
#include <CORBA/Proxy/IORequest_s.h>
#include <CORBA/LifeCycleNoCopy.h>
#include "ServantMarshaler.h"

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
	Bridge <Object>* _get_object (String_in iid)
	{
		if (RepositoryId::check (Object::repository_id_, iid) != RepositoryId::COMPATIBLE)
			::Nirvana::throw_INV_OBJREF ();
		return this;
	}

	Object_ptr get_proxy ()
	{
		return &static_cast <Object&> (static_cast <Bridge <Object>&> (*this));
	}

	typedef ::Nirvana::Core::AtomicCounter <false> RefCnt;

	void _add_ref ()
	{
		RefCnt::IntegralType cnt = ref_cnt_.increment ();
		if (1 == cnt) {
			try {
				::Nirvana::Core::Synchronized sync (*sync_context_);
				add_ref_1 ();
			} catch (...) {
				ref_cnt_.decrement ();
				throw;
			}
		}
	}

	virtual RefCnt::IntegralType _remove_ref ();

protected:
	ServantProxyBase (AbstractBase_ptr servant, const Operation object_ops [3], void* object_impl);

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
			// TODO: Garbage collector deadline must not be infinite, but reasonable enough.
			::Nirvana::Core::ExecDomain::async_call <GC> (::Nirvana::INFINITE_DEADLINE, sync_context_->sync_domain (), std::forward <Args> (args)...);
		} catch (...) {
			// Async call failed, maybe resources are exausted.
			// Fallback to collect garbage in current thread.
			try {
				::Nirvana::Core::ImplStatic <GC> (std::forward <Args> (args)...).run ();
			} catch (...) {
				// Swallow exceptions.
				// TODO: Log error.
			}
		}
	}

	template <class I, void (*proc) (I*, IORequest_ptr, ::Nirvana::ConstPointer, Unmarshal_var, ::Nirvana::Pointer)>
	static void ObjProcWrapper (Interface* servant, Interface* call,
		::Nirvana::ConstPointer in_params,
		Interface** unmarshaler,
		::Nirvana::Pointer out_params)
	{
		try {
			IORequest_ptr rq = IORequest::_check (call);
			try {
				proc ((I*)(void*)servant, rq, in_params, TypeI <Unmarshal>::inout (unmarshaler), out_params);
				rq->success ();
			} catch (Exception& e) {
				rq->exception (std::move (e));
			}
		} catch (...) {
		}
	}

	class Request :
		public ImplementationPseudo <Request, IORequest>,
		public LifeCycleNoCopy <Request>
	{
	public:
		Request (const ServantProxyBase& proxy) :
			proxy_ (proxy),
			success_ (false)
		{
			exception_.reset ();
		}

		Marshal_ptr marshaler ()
		{
			if (!marshaler_)
				marshaler_ = proxy_.create_marshaler ();
			return marshaler_;
		}

		void exception (Any& exc)
		{
			TypeCode_ptr tc = exc.type ();
			if (tc)
				MarshalTraits <Any>::marshal_out (exc, marshaler (), exception_);
		}

		void success ()
		{
			success_ = true;
		}

		Unmarshal_var check ()
		{
			Unmarshal_var u = ServantMarshaler::unmarshaler (marshaler_._retn ());
			if (!success_) {
				if (exception_.type ()) {
					Any exc;
					MarshalTraits <Any>::unmarshal (exception_, u, exc);
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
		const ServantProxyBase& proxy_;
		Marshal_var marshaler_;
		ABI <Any> exception_;
		bool success_;
	};

public:
	Marshal_var create_marshaler () const
	{
		return (new ServantMarshaler (sync_context_))->marshaler ();
	}

	Unmarshal_var call (OperationIndex op,
		const void* in_params, size_t in_params_size,
		Marshal_var& marshaler,
		void* out_params, size_t out_params_size)
	{
		::Nirvana::UWord idx = op.interface_idx;
		if (idx >= interfaces ().size ())
			throw BAD_OPERATION ();
		const InterfaceEntry& ie = interfaces () [idx];
		idx = op.operation_idx;
		if (idx >= ie.operations.size)
			throw BAD_OPERATION ();
		Unmarshal_var u = ServantMarshaler::unmarshaler (marshaler);
		marshaler._retn ();
		Request request (*this);
		{
			::Nirvana::Core::Synchronized sync (get_sync_context (op));
			(ie.operations.p [idx].invoke) (ie.implementation, &request._get_ptr (), in_params, &TypeI <Unmarshal>::C_inout (u), out_params);
		}
		return request.check ();
	}

private:
	static const Char* primary_interface_id (AbstractBase_ptr servant)
	{
		Interface* primary = servant->_query_interface (0);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

private:
	AbstractBase_ptr servant_;
	RefCnt ref_cnt_;
	::Nirvana::Core::Core_var <::Nirvana::Core::SyncContext> sync_context_;
};

}
}
}

#endif
