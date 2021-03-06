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
namespace Internal {
namespace Core {

/// Object operations proxy.
class ProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;
	class Deactivator;

protected:
	ProxyObject (PortableServer::Servant servant) :
		ServantProxyBase (servant, object_ops_, this),
		implicit_activation_ (false)
	{}

	~ProxyObject ()
	{
		assert (implicit_activated_id_.empty ());
		release_object_id (implicit_activated_id_);
	}

	PortableServer::Servant servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <PortableServer::ServantBase*> (&Base::servant ());
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

	static void non_existent_request (ProxyObject* servant, IORequest::_ptr_type call);

	PortableServer::ServantBase::_ref_type _get_servant () const
	{
		if (&sync_context () == &::Nirvana::Core::SyncContext::current ())
			return servant ();
		else
			throw MARSHAL ();
	}

	static Interface* __get_servant (Bridge <Object>* obj, Interface* env)
	{
		try {
			return Type <PortableServer::Servant>::ret (static_cast <ProxyObject&> (_implementation (obj))._get_servant ());
		} catch (Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return 0;
	}

	void release_object_id (PortableServer::ObjectId& oid) const NIRVANA_NOEXCEPT;

private:
	std::atomic <ActivationState> activation_state_;
	PortableServer::ObjectId implicit_activated_id_;
	bool implicit_activation_;

	static const Operation object_ops_ [3];
};

}
}
}

#endif
