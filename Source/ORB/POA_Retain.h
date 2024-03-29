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
	virtual void deactivate_object (ObjectId& oid) override;

	virtual CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type reference) override;
	virtual CORBA::Object::_ref_type id_to_servant (ObjectId& oid) override;
	virtual CORBA::Object::_ref_type id_to_reference (ObjectId& oid) override;

	enum DGC_Policy
	{
		DGC_DEFAULT,
		DGC_ENABLED,
		DGC_DISABLED
	};

	DGC_Policy DGC_policy () const noexcept
	{
		return DGC_policy_;
	}

	virtual unsigned get_flags (unsigned flags) const noexcept override;

protected:
	POA_Retain ();

	virtual CORBA::Core::ReferenceLocalRef activate_object (ObjectId&& oid, bool unique,
		CORBA::Core::ServantProxyObject& proxy, unsigned flags) override;

	virtual void activate_object (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) override;

	virtual void serve_request (Request& request, const ObjectId& oid,
		CORBA::Core::ReferenceLocal* reference) override;

	virtual void deactivate_objects (bool etherealize) noexcept override;

	virtual void etherialize (const CORBA::Core::ReferenceLocal& ref, CORBA::Core::ServantProxyObject& proxy,
		bool cleanup_in_progress) noexcept
	{};

	virtual void implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) noexcept override;
	virtual CORBA::servant_reference <CORBA::Core::ServantProxyObject> deactivate_reference (
		CORBA::Core::ReferenceLocal& ref, bool etherealize, bool cleanup_in_progress) override;

private:
	// Set of the active references
	typedef Nirvana::Core::PointerSet <CORBA::Core::ReferenceLocal> References;
	References active_references_;

	DGC_Policy DGC_policy_;
};

}
}

#endif
