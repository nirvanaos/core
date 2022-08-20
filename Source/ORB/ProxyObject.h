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
#include <atomic>

namespace CORBA {
namespace Core {

/// Object operations servant-side proxy.
class ProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;
	class Deactivator;

public:
	/// Called from the POA synchronization domain.
	/// 
	/// \param poa Activation POA
	/// \param oid Pointer to the activated ObjectId. 
	///   `nullptr` if servant activated via set_servant.
	/// \param implicit `true` for implicit activation, `false` for explicit activation.
	void activate (PortableServer::POA::_ptr_type poa,
		const PortableServer::ObjectId* oid, bool implicit) NIRVANA_NOEXCEPT
	{
		assert (!activated_POA_);
		activated_POA_ = poa;
		activated_id_ = oid;
		implicit_activation_ = implicit;
		activation_state_ = ActivationState::ACTIVE;
	}

	// Called from the POA synchronization domain
	PortableServer::POA::_ptr_type activated_POA () const NIRVANA_NOEXCEPT
	{
		return activated_POA_;
	}

	// Called from the POA synchronization domain
	const PortableServer::ObjectId* activated_id () const NIRVANA_NOEXCEPT
	{
		return activated_id_;
	}

	// Called from the POA synchronization domain
	void deactivate () NIRVANA_NOEXCEPT
	{
		activated_POA_ = nullptr;
		activated_id_ = nullptr;
		implicit_activation_ = false;
		activation_state_ = ActivationState::INACTIVE;
	}

	// Returns user ServantBase implementation
	PortableServer::Servant servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <PortableServer::ServantBase*> (&Base::servant ());
	}

protected:
	ProxyObject (PortableServer::Servant servant) :
		ServantProxyBase (servant, object_ops_, this),
		activation_state_ (ActivationState::INACTIVE),
		activated_id_ (nullptr),
		implicit_activation_ (false)
	{}

	~ProxyObject ()
	{
		assert (!activated_id_);
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

	bool change_state (ActivationState from, ActivationState to) NIRVANA_NOEXCEPT
	{
		return activation_state_.compare_exchange_strong (from, to);
	}

	static void non_existent_request (ProxyObject* servant, Internal::IORequest::_ptr_type call);

private:
	std::atomic <ActivationState> activation_state_;
	PortableServer::POA::_ref_type activated_POA_;
	const PortableServer::ObjectId* activated_id_;
	bool implicit_activation_;

	static const OperationsDII object_ops_;
};

}
}

#endif
