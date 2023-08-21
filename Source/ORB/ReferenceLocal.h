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
#ifndef NIRVANA_ORB_CORE_REFERENCELOCAL_H_
#define NIRVANA_ORB_CORE_REFERENCELOCAL_H_
#pragma once

#include "Reference.h"
#include "ServantBase.h"
#include "ObjectKey.h"

namespace CORBA {
namespace Core {

/// Base for all POA references.
class ReferenceLocal :
	public Reference
{
public:
	ReferenceLocal (const IOP::ObjectKey& object_key, PortableServer::Core::ObjectKey&& core_key,
		Internal::String_in primary_iid, unsigned flags, PolicyMapShared* policies);
	ReferenceLocal (const IOP::ObjectKey& object_key, PortableServer::Core::ObjectKey&& core_key,
		ServantProxyObject& proxy, unsigned flags, PolicyMapShared* policies);

	~ReferenceLocal ();

	const IOP::ObjectKey& object_key () const noexcept
	{
		return object_key_;
	}

	const PortableServer::Core::ObjectKey& core_key () const noexcept
	{
		return core_key_;
	}

	void activate (ServantProxyObject& proxy);
	servant_reference <ServantProxyObject> deactivate () noexcept;

	void on_servant_destruct () noexcept;

	virtual void _add_ref () noexcept override;
	virtual void _remove_ref () noexcept override;

	servant_reference <ServantProxyObject> get_active_servant () const noexcept;
	servant_reference <ServantProxyObject> get_active_servant_with_lock () const noexcept;
	PortableServer::ServantBase::_ref_type _get_servant () const override;

	static void marshal (const ProxyManager& proxy, const Octet* obj_key, size_t obj_key_size,
		const PolicyMap* policies, StreamOut& out);

	virtual DomainManagersList _get_domain_managers () override;

	virtual Boolean _is_equivalent (Object::_ptr_type other_object) const noexcept override;

	virtual ReferenceRef marshal (StreamOut& out) override;
	virtual ReferenceLocalRef get_local_reference (const PortableServer::Core::POA_Base&) override;
	virtual Internal::IORequest::_ref_type create_request (Internal::OperationIndex op, unsigned flags,
		CallbackRef&& callback) override;

private:
	static void marshal_object_key (const Octet* obj_key, size_t obj_key_size, StreamOut& stream);

private:
	const PortableServer::Core::ObjectKey core_key_;
	const IOP::ObjectKey& object_key_;

	servant_reference <Nirvana::Core::SyncContext> adapter_context_;

	static const size_t SERVANT_ALIGN = Nirvana::Core::core_object_align (
		sizeof (PortableServer::Core::ServantBaseImpl));
	typedef Nirvana::Core::LockablePtrT <ServantProxyObject, 0, SERVANT_ALIGN> ServantPtr;

	mutable ServantPtr servant_;
};

typedef servant_reference <ReferenceLocal> ReferenceLocalRef;

}
}

#endif
