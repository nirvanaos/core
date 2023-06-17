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
#ifndef NIRVANA_NAMESERVICE_NAMINGCONTEXTBASE_H_
#define NIRVANA_NAMESERVICE_NAMINGCONTEXTBASE_H_
#pragma once

#include <CORBA/CORBA.h>
#include <CORBA/CosNaming.h>
#include <memory>

namespace CosNaming {
namespace Core {

class Iterator;

class NIRVANA_NOVTABLE NamingContextBase
{
public:
	void bind (Name& n, CORBA::Object::_ptr_type obj);
	virtual void bind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) = 0;

	void rebind (Name& n, CORBA::Object::_ptr_type obj);
	virtual void rebind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) = 0;

	void bind_context (Name& n, NamingContext::_ptr_type nc);
	virtual void bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) = 0;

	void rebind_context (Name& n, NamingContext::_ptr_type nc);
	virtual void rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) = 0;

	CORBA::Object::_ref_type resolve (Name& n);
	virtual CORBA::Object::_ref_type resolve1 (const Istring& name, BindingType& type, Name& n) = 0;

	void unbind (Name& n);
	virtual void unbind1 (const Istring& name, Name& n) = 0;

	NamingContext::_ref_type bind_new_context (Name& n);
	virtual NamingContext::_ref_type bind_new_context1 (Istring&& name, Name& n) = 0;

	void list (uint32_t how_many, BindingList& bl, CosNaming::BindingIterator::_ref_type& bi) const;
	virtual std::unique_ptr <Iterator> make_iterator () const = 0;

	static Istring to_string (const NameComponent& nc);
	static void check_name (const Name& n);

protected:
	virtual NamingContext::_ref_type create_context1 (Istring&& name, Name& n, bool& created) = 0;

private:
	NamingContext::_ref_type resolve_context (Name& n);
	NamingContext::_ref_type create_context (Name& n, Name& created_name);
	void unbind_created (Name& created) noexcept;
};

}
}

#endif
