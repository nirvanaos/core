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
#include "../MapUnorderedUnstable.h"

namespace PortableServer {
namespace Core {

// Active Object Map (AOM) value.
class AOM_Val : public CORBA::Object::_ref_type
{
	typedef CORBA::Object::_ref_type Base;
public:
	AOM_Val (CORBA::Object::_ptr_type p) :
		Base (p)
	{}

	AOM_Val (const AOM_Val&) = delete;
	AOM_Val (AOM_Val&&) = default;

	AOM_Val& operator = (const AOM_Val&) = delete;
	AOM_Val& operator = (AOM_Val&&) = default;

	~AOM_Val ()
	{
		if (p_) // Not moved
			object2servant_core (p_)->deactivate ();
	}
};

class NIRVANA_NOVTABLE POA_Retain : public POA_Base
{
	typedef POA_Base Base;
public:
	virtual void destroy (bool etherealize_objects, bool wait_for_completion) override;
	virtual void deactivate_object (const ObjectId& oid) override;
	virtual CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) override;

protected:
	POA_Retain (CORBA::servant_reference <POAManager>&& manager) :
		Base (std::move (manager))
	{}

	~POA_Retain ()
	{}

	virtual void serve (CORBA::Core::RequestInBase& request) const override;

protected:
	// Active Object Map (AOM)
	typedef Nirvana::Core::MapUnorderedUnstable <ObjectId, AOM_Val,
		std::hash <ObjectId>, std::equal_to <ObjectId>,
		Nirvana::Core::UserAllocator <std::pair <ObjectId, AOM_Val> > > AOM;

	AOM active_object_map_;
};

}
}

#endif
