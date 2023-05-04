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
#ifndef NIRVANA_ORB_CORE_SERVANTPROXYOBJECT_H_
#define NIRVANA_ORB_CORE_SERVANTPROXYOBJECT_H_
#pragma once

#include "ServantProxyBase.h"
#include "../TaggedPtr.h"
#include "../PointerSet.h"
#include "Reference.h"

namespace CORBA {
namespace Core {

class ReferenceLocal;
typedef servant_reference <ReferenceLocal> ReferenceLocalRef;

/// \brief Server-side Object proxy.
class NIRVANA_NOVTABLE ServantProxyObject :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;

public:
	typedef PortableServer::ServantBase ServantInterface;

	virtual void _add_ref () override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;

	///@{
	/// Called from the POA synchronization domain.
	/// So calls to activate (), deactivate () and is_active () are always serialized.
	void activate (ReferenceLocal& reference)
	{
		ReferenceLocal* old = reference_.exchange (&reference);
		// If the set is empty, object can have one reference stored in reference_.
		// If the set is not empty, it must contain all the references include stored in reference_.
		if (old) {
			references_.insert (old);
			references_.insert (&reference);
		}
	}

	void deactivate (ReferenceLocal& reference) NIRVANA_NOEXCEPT
	{
		references_.erase (&reference);
		ReferenceLocal* p = references_.empty () ? nullptr : reinterpret_cast <ReferenceLocal*> (*references_.begin ());
		reference_.cas (&reference, p);
	}

	bool is_active () const NIRVANA_NOEXCEPT
	{
		return static_cast <ReferenceLocal*> (reference_.load ()) != nullptr;
	}
	///@}

	/// Returns user ServantBase implementation
	PortableServer::Servant servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <PortableServer::ServantBase*> (&Base::servant ());
	}

	void weak_lock ()
	{
		_add_ref ();
	}

	void weak_unlock ()
	{
		_remove_ref ();
	}

	Nirvana::Core::Ref <ServantProxyObject> weak_get_ref ()
	{
		return this;
	}

protected:
	ServantProxyObject (PortableServer::Servant user_servant);
	~ServantProxyObject ();

	virtual Boolean non_existent () override;
	virtual ReferenceRef get_reference () override;

	ReferenceLocalRef get_reference_local () const NIRVANA_NOEXCEPT;

	virtual Policy::_ref_type _get_policy (PolicyType policy_type) override;
	virtual DomainManagersList _get_domain_managers () override;

protected:
	Nirvana::Core::Ref <Nirvana::Core::SyncContext> adapter_context_;

	static const size_t REF_ALIGN = Nirvana::Core::core_object_align (sizeof (Reference));
	typedef Nirvana::Core::LockablePtrT <ReferenceLocal, 0, REF_ALIGN> RefPtr;

	mutable RefPtr reference_;

	// The set of references.
	// If the set is empty, object can have one reference stored in reference_.
	// If the set is not empty, it contains all the references include stored in reference_.
	Nirvana::Core::PointerSet references_;
};

CORBA::Object::_ptr_type servant2object (PortableServer::Servant servant) NIRVANA_NOEXCEPT;

inline
CORBA::Core::ServantProxyObject* object2proxy (CORBA::Object::_ptr_type obj) NIRVANA_NOEXCEPT
{
	return static_cast <ServantProxyObject*> (ProxyManager::cast (obj));
}

PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj);

}
}

#endif
