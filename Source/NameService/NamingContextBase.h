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
#ifndef NIRVANA_NAMESERVICE_NAMINGCONTEXTBASE_H_
#define NIRVANA_NAMESERVICE_NAMINGCONTEXTBASE_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"

namespace CosNaming {
namespace Core {

class NamingContextBase
{
public:
	void bind (Name& n, CORBA::Object::_ptr_type obj);
	void rebind (Name& n, CORBA::Object::_ptr_type obj);
	void bind_context (Name& n, NamingContext::_ptr_type nc);
	void rebind_context (Name& n, NamingContext::_ptr_type nc);
	CORBA::Object::_ref_type resolve (Name& n) const;
	void unbind (Name& n);
	NamingContext::_ref_type new_context ();
	NamingContext::_ref_type bind_new_context (Name& n);
	void destroy ();
	void list (uint32_t how_many, BindingList& bl, BindingIterator::_ref_type& bi);

	static Istring to_string (const NameComponent& nc);
	static NameComponent to_component (const Istring& s);

private:
	NamingContext::_ref_type resolve_context (Name& n) const;
	NamingContext::_ref_type create_context (Name& n, Istring& created);

protected:
	struct MapVal
	{
		CORBA::Object::_ref_type object;
		BindingType binding_type;

		MapVal (CORBA::Object::_ptr_type obj, BindingType type) :
			object (obj),
			binding_type (type)
		{}

		MapVal ()
		{}
	};

	using Bindings = Nirvana::Core::MapUnorderedUnstable <Istring, MapVal,
		std::hash <Istring>, std::equal_to <Istring>, Nirvana::Core::UserAllocator>;

	Bindings bindings_;
};

}
}

#endif
