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
	public POA_ImplicitRetain,
	public POA_System
{};

class POA_ImplicitUniqueTransient :
	public POA_ImplicitUnique,
	public POA_System
{};

class POA_ImplicitPersistent :
	public POA_ImplicitRetain,
	public POA_SystemPersistent
{};

class POA_ImplicitUniquePersistent :
	public POA_ImplicitUnique,
	public POA_SystemPersistent
{};

}
}

#endif
