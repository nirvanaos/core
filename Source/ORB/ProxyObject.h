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

#include "ServantProxyBase.h"
#include "ObjectKey.h"
#include "../LockableRef.h"
#include <atomic>

namespace CORBA {
namespace Core {

/// Server-side object reference.
class ProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;
	class Deactivator;

public:
	///@{
	/// Called from the POA synchronization domain.
	/// So calls to activate () and deactivate () are always serialized.
	void activate (PortableServer::Core::ObjectKeyRef&& key) NIRVANA_NOEXCEPT;
	void deactivate () NIRVANA_NOEXCEPT;

	PortableServer::Core::ObjectKeyRef object_key () NIRVANA_NOEXCEPT
	{
		return object_key_.get ();
	}

	///@}

	/// Returns user ServantBase implementation
	PortableServer::Servant servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <PortableServer::ServantBase*> (&Base::servant ());
	}

	/// The unique stamp is assigned when object reference is created.
	/// The timestamp together with the ProxyObject pointer let to create
	/// the unique system id for the reference.
	typedef int Timestamp;
	Timestamp timestamp () const NIRVANA_NOEXCEPT
	{
		return timestamp_;
	}

	static void initialize () NIRVANA_NOEXCEPT
	{
		next_timestamp_ = (Timestamp)(Nirvana::Core::Chrono::UTC ().time () / TimeBase::SECOND);
	}

protected:
	ProxyObject (PortableServer::Servant servant);
	ProxyObject (const ProxyObject& src);

	~ProxyObject ()
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

	virtual void add_ref_1 () override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	template <class> friend class Nirvana::Core::CoreRef;

	void implicit_deactivate ();

	void marshal_object_key (StreamOut& out)
	{
		PortableServer::Core::ObjectKeyRef ref = object_key_.get ();
		if (!ref)
			throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
		ref->marshal (out);
	}

	bool change_state (ActivationState from, ActivationState to) NIRVANA_NOEXCEPT
	{
		return activation_state_.compare_exchange_strong (from, to);
	}

	virtual Boolean non_existent () override;

private:
	Timestamp timestamp_;
	std::atomic <ActivationState> activation_state_;
	PortableServer::POA::_ref_type implicit_POA_;
	Nirvana::Core::LockableRef <PortableServer::Core::ObjectKeyImpl> object_key_;

	static std::atomic <Timestamp> next_timestamp_;
};

template <class Base>
class ProxyObjectImpl :
	public Base
{
	template <class> friend class Nirvana::Core::CoreRef;

	template <class ... Args>
	ProxyObjectImpl (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}

	virtual void _remove_ref () NIRVANA_NOEXCEPT override
	{
		if (0 == Base::_remove_ref_proxy ())
			delete this;
	}
};

CORBA::Object::_ptr_type servant2object (PortableServer::Servant servant) NIRVANA_NOEXCEPT;

inline
CORBA::Core::ProxyObject* object2proxy (CORBA::Object::_ptr_type obj) NIRVANA_NOEXCEPT
{
	return static_cast <CORBA::Core::ProxyObject*> (
		static_cast <CORBA::Internal::Bridge <CORBA::Object>*> (&obj));
}

PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj);

}
}

#endif
