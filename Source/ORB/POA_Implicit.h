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

#include "POA_RetainSystem.h"

namespace PortableServer {
namespace Core {

// POA with IMPLICIT_ACTIVATION
class POA_Implicit :
	public POA_RetainSystem
{
	typedef POA_RetainSystem Base;

public:
	virtual bool implicit_activation () const NIRVANA_NOEXCEPT override
	{
		return true;
	}

	virtual ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant) override;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant) override;

protected:
	POA_Implicit (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		Base (parent, name, std::move (manager))
	{}
};

// POA with IMPLICIT_ACTIVATION and UNIQUE_ID
class POA_ImplicitUnique : public POA_RetainSystemUnique
{
	typedef POA_RetainSystemUnique Base;

public:
	virtual bool implicit_activation () const NIRVANA_NOEXCEPT override
	{
		return true;
	}

	virtual ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant) override;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant) override;

protected:
	POA_ImplicitUnique (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		Base (parent, name, std::move (manager))
	{}

private:
	bool AOM_insert (const ObjectId& oid, CORBA::Object::_ptr_type p_servant);
};

}
}

#endif
