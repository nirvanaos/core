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

namespace PortableServer {
namespace Core {

class POA_Root;

}
}

namespace CORBA {
namespace Core {

/// Base for all POA references.
class ReferenceLocal :
	public PortableServer::Core::ObjectKey,
	public Reference
{
public:
	ReferenceLocal (PortableServer::Core::ObjectKey&& key, const IDL::String& primary_iid,
		bool garbage_collection);
	ReferenceLocal (PortableServer::Core::ObjectKey&& key, PortableServer::Core::ServantBase& servant,
		bool garbage_collection);

	void activate (PortableServer::Core::ServantBase& servant);
	servant_reference <ProxyObject> deactivate () NIRVANA_NOEXCEPT;

	void on_delete_implicit (PortableServer::Core::ServantBase& servant) NIRVANA_NOEXCEPT;

	virtual void _add_ref () override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;

	Nirvana::Core::CoreRef <ProxyObject> get_servant () const NIRVANA_NOEXCEPT;

private:
	virtual ReferenceLocal* local_reference () override;

	virtual void marshal (StreamOut& out) const override;
	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, UShort flags) override;

	class GC;

private:
	servant_reference <PortableServer::Core::POA_Root> root_;

	static const size_t SERVANT_ALIGN = Nirvana::Core::core_object_align (
		sizeof (PortableServer::Core::ServantBase) + sizeof (ProxyObject));
	typedef Nirvana::Core::LockablePtrT <PortableServer::Core::ServantBase, 0, SERVANT_ALIGN> ServantPtr;

	mutable ServantPtr servant_;
};

typedef servant_reference <ReferenceLocal> ReferenceLocalRef;

}
}

#endif
