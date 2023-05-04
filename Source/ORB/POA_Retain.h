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
#ifndef NIRVANA_ORB_CORE_POA_RETAIN_H_
#define NIRVANA_ORB_CORE_POA_RETAIN_H_
#pragma once

#include "POA_Base.h"
#include "../PointerSet.h"

namespace PortableServer {
namespace Core {

// RETAIN policy
class NIRVANA_NOVTABLE POA_Retain : public virtual POA_Base
{
public:
	virtual void deactivate_object (const ObjectId& oid) override;

	virtual CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type reference) override;
	virtual CORBA::Object::_ref_type id_to_servant (const ObjectId& oid) override;
	virtual CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) override;

protected:
	virtual CORBA::Core::ReferenceLocalRef activate_object (ObjectKey&& key, bool unique,
		CORBA::Core::ServantProxyObject& proxy, unsigned flags) override;

	virtual void activate_object (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) override;

	virtual void serve (const RequestRef& request, CORBA::Core::ReferenceLocal& reference) override;
	virtual void destroy_internal (bool etherealize_objects) NIRVANA_NOEXCEPT override;
	virtual void etherealize_objects () NIRVANA_NOEXCEPT override;
	virtual void implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) NIRVANA_NOEXCEPT override;
	virtual CORBA::servant_reference <CORBA::Core::ServantProxyObject> deactivate_object (
		CORBA::Core::ReferenceLocal& ref) override;

private:
	// This map contains active references and references with GC
	typedef Nirvana::Core::PointerSet References;
	References references_;
};

}
}

#endif
