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
#ifndef NIRVANA_ORB_CORE_PROXYOBJECT_H_
#define NIRVANA_ORB_CORE_PROXYOBJECT_H_
#pragma once

#include "ServantProxyBase.inl"
#include "ObjectKey.h"
#include "../LockableRef.h"
#include <atomic>

namespace CORBA {
namespace Core {

struct ActivationKey :
	public PortableServer::Core::ObjectKey,
	public Nirvana::Core::UserObject
{};

typedef Nirvana::Core::ImplDynamic <ActivationKey> ActivationKeyImpl;
typedef Nirvana::Core::CoreRef <ActivationKeyImpl> ActivationKeyRef;

/// Object operations servant-side proxy.
class ProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;
	class Deactivator;

public:
	///@{
	/// Called from the POA synchronization domain.
	/// So calls to activate() and deactivate() are always serialized.
	/// NOTE: The ActivationKey structure is allocated in the POA memory context.
	/// And must be released in the POA memory context.
	void activate (ActivationKeyRef&& key) NIRVANA_NOEXCEPT;
	void deactivate () NIRVANA_NOEXCEPT;

	ActivationKeyRef get_object_key () NIRVANA_NOEXCEPT
	{
		ActivationKeyRef ref = activation_key_.get ();
		assert (!ref || activation_key_memory_ == &Nirvana::Core::MemContext::current ());
		return ref;
	}

	///@}

	// Returns user ServantBase implementation
	PortableServer::Servant servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <PortableServer::ServantBase*> (&Base::servant ());
	}

protected:
	ProxyObject (PortableServer::Servant servant) :
		ServantProxyBase (servant, object_ops_, this),
		activation_state_ (ActivationState::INACTIVE)
	{}

	~ProxyObject ()
	{
		assert (!activation_key_.get ());
	}

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
	virtual RefCnt::IntegralType _remove_ref_proxy () NIRVANA_NOEXCEPT;

	void implicit_deactivate ();

	void marshal_object_key (StreamOut& out)
	{
		ActivationKeyRef ref = activation_key_.get ();
		if (!ref)
			throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
		Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
		ed.mem_context_push (activation_key_memory_);
		try {
			ref->marshal (out);
		} catch (...) {
			ed.mem_context_pop ();
			throw;
		}
		ed.mem_context_pop ();
	}

	bool change_state (ActivationState from, ActivationState to) NIRVANA_NOEXCEPT
	{
		return activation_state_.compare_exchange_strong (from, to);
	}

	static void non_existent_request (ProxyObject* servant, Internal::IORequest::_ptr_type call);

private:
	std::atomic <ActivationState> activation_state_;
	PortableServer::POA::_ref_type implicit_POA_;
	Nirvana::Core::LockableRef <ActivationKeyImpl> activation_key_;
	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> activation_key_memory_;

	static const OperationsDII object_ops_;
};

}
}

#endif
