#ifndef NIRVANA_ORB_CORE_PROXYOBJECT_H_
#define NIRVANA_ORB_CORE_PROXYOBJECT_H_

#include "ServantProxyBase.h"
#include <atomic>

namespace CORBA {
namespace Nirvana {
namespace Core {

class ProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;
	class Deactivator;

protected:
	ProxyObject (PortableServer::Servant servant) :
		ServantProxyBase (servant, object_ops_, this),
		servant_ (servant),
		implicit_activation_ (false)
	{}

private:
	enum ActivationState
	{
		INACTIVE,
		ACTIVATION,
		ACTIVE,
		DEACTIVATION_SCHEDULED,
		DEACTIVATION_CANCELLED
	};

	virtual void add_ref_1 ();
	virtual ::Nirvana::Core::RefCounter::UIntType _remove_ref ();

	void implicit_deactivate ();

	bool change_state (ActivationState from, ActivationState to)
	{
		return activation_state_.compare_exchange_strong (from, to);
	}

	static void non_existent_request (ProxyObject* servant,
		IORequest_ptr call,
		::Nirvana::ConstPointer in_params,
		Unmarshal_var unmarshaler,
		::Nirvana::Pointer out_params);

	PortableServer::ServantBase_var _get_servant () const
	{
		if (&sync_context () == &::Nirvana::Core::SynchronizationContext::current ())
			return PortableServer::ServantBase::_duplicate (servant_);
		else
			throw MARSHAL ();
	}

	static Interface* __get_servant (Bridge <Object>* obj, Interface* env)
	{
		try {
			return TypeI <PortableServer::ServantBase>::ret (static_cast <ProxyObject&> (_implementation (obj))._get_servant ());
		} catch (const Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return 0;
	}

private:
	PortableServer::Servant servant_;
	std::atomic <ActivationState> activation_state_;
	String implicit_activated_id_;
	bool implicit_activation_;

	static const Operation object_ops_ [3];
};

}
}
}

#endif
