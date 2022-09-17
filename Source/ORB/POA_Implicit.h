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
#ifndef NIRVANA_ORB_CORE_POA_IMPLICIT_H_
#define NIRVANA_ORB_CORE_POA_IMPLICIT_H_
#pragma once

#include "POA_Unique.h"
#include "POA_System.h"

namespace PortableServer {
namespace Core {

// POA with IMPLICIT_ACTIVATION
class POA_Implicit :
	public virtual POA_Base
{
public:
	virtual bool implicit_activation () const NIRVANA_NOEXCEPT override
	{
		return true;
	}

	virtual ObjectId servant_to_id_default (CORBA::Core::ProxyObject& proxy, bool not_found) override;
	virtual CORBA::Object::_ref_type servant_to_reference_default (CORBA::Core::ProxyObject& proxy, bool not_found) override;
};

class POA_ImplicitRetain :
	public POA_Implicit,
	public POA_Retain
{};

class POA_ImplicitUnique :
	public POA_Implicit,
	public POA_Unique
{};


class POA_ImplicitTransient :
	public virtual POA_Base,
	public POA_ImplicitRetain,
	public POA_System
{
protected:
	POA_ImplicitTransient ()
	{}

	POA_ImplicitTransient (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

class POA_ImplicitUniqueTransient :
	public virtual POA_Base,
	public POA_ImplicitUnique,
	public POA_System
{
protected:
	POA_ImplicitUniqueTransient ()
	{}

	POA_ImplicitUniqueTransient (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

class POA_ImplicitPersistent :
	public virtual POA_Base,
	public POA_ImplicitRetain,
	public POA_SystemPersistent
{
protected:
	POA_ImplicitPersistent ()
	{}

	POA_ImplicitPersistent (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

class POA_ImplicitUniquePersistent :
	public virtual POA_Base,
	public POA_ImplicitUnique,
	public POA_SystemPersistent
{
protected:
	POA_ImplicitUniquePersistent ()
	{}

	POA_ImplicitUniquePersistent (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

}
}

#endif
