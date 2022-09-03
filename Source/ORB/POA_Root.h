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
#ifndef NIRVANA_ORB_CORE_POA_ROOT_H_
#define NIRVANA_ORB_CORE_POA_ROOT_H_
#pragma once

#include "POA_Implicit.h"
#include "POAManagerFactory.h"

namespace PortableServer {
namespace Core {

class POA_Root :
	public POA_ImplicitUnique
{
	typedef POA_ImplicitUnique Base;
public:
	POA_Root (CORBA::servant_reference <POAManager>&& manager,
		CORBA::servant_reference <POAManagerFactory>&& manager_factory) :
		Base (std::move (manager)),
		manager_factory_ (std::move (manager_factory))
	{}

	~POA_Root ()
	{
		if (!is_destroyed ())
			destroy (true, false);
	}

	virtual IDL::String the_name () const override
	{
		return "RootPOA";
	}

	virtual CORBA::OctetSeq id () const override
	{
		return CORBA::OctetSeq ();
	}

	static void invoke (CORBA::Core::RequestInBase& request) NIRVANA_NOEXCEPT;

	static PortableServer::POA::_ref_type create ()
	{
		auto manager_factory = CORBA::make_reference <POAManagerFactory> ();
		auto manager = manager_factory->create ("RootPOAManager", CORBA::PolicyList ());
		return CORBA::make_reference <POA_Root> (std::move (manager), std::move (manager_factory))->_this ();
	}

	static POA::_ref_type get_root ();

private:
	CORBA::servant_reference <POAManagerFactory> manager_factory_;

	static POA* root_;
};

}
}

#endif
