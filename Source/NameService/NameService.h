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

#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>

namespace CosNaming {
namespace Core {

/// Naming Service root
class NameService :
	public CORBA::servant_traits <NamingContextExt>::Servant <NameService>
{
public:
	// NamingContext

	void bind (Name& n, CORBA::Object::_ptr_type obj)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void rebind (Name& n, CORBA::Object::_ptr_type obj)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void bind_context (Name& n, NamingContext::_ptr_type nc)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void rebind_context (Name& n, NamingContext::_ptr_type nc)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Object::_ref_type resolve (const Name& n)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void unbind (const Name& n)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NamingContext::_ref_type new_context ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NamingContext::_ref_type bind_new_context (Name& n)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void destroy ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void list (uint32_t how_many, BindingList& bl, BindingIterator::_ref_type& bi)
	{
		throw CORBA::NO_IMPLEMENT ();
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
		throw CORBA::NO_IMPLEMENT ();
	}

};

}
}

#endif
