/// \file
/*
* Nirvana IDL support library.
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
#ifndef NIRVANA_NAMESERVICE_NAMESERVICE_H_
#define NIRVANA_NAMESERVICE_NAMESERVICE_H_
#pragma once

#include "NamingContextBase.h"
#include "ORB/SysServant.h"
#include "ORB/system_services.h"

namespace CosNaming {
namespace Core {

/// Naming Service root
class NameService : 
	public CORBA::Core::SysServantImpl <NameService, NamingContextExt, NamingContext>,
	public NamingContextBase
{
public:
	// NamingContext

	void bind (Name& n, CORBA::Object::_ptr_type obj)
	{
		NamingContextBase::bind (n, obj);
	}

	void rebind (Name& n, CORBA::Object::_ptr_type obj)
	{
		NamingContextBase::rebind (n, obj);
	}

	void bind_context (Name& n, NamingContext::_ptr_type nc)
	{
		NamingContextBase::bind_context (n, nc);
	}

	void rebind_context (Name& n, NamingContext::_ptr_type nc)
	{
		NamingContextBase::rebind_context (n, nc);
	}

	CORBA::Object::_ref_type resolve (Name& n)
	{
		return NamingContextBase::resolve (n);
	}

	void unbind (Name& n)
	{
		NamingContextBase::unbind (n);
	}

	NamingContext::_ref_type bind_new_context (Name& n)
	{
		return NamingContextBase::bind_new_context (n);
	}

	static void destroy ()
	{
		throw NotEmpty ();
	}

	void list (uint32_t how_many, BindingList& bl, BindingIterator::_ref_type& bi)
	{
		NamingContextBase::list (how_many, bl, bi);
	}

	// NamingContextEx

	static StringName to_string (const Name& n)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static Name to_name (const StringName& sn)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static URLString to_url (const Address& addr, const StringName& sn)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Object::_ref_type resolve_str (const StringName& sn)
	{
		Name name = to_name (sn);
		return resolve (name);
	}

};

}
}

#endif
