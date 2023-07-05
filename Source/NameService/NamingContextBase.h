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

#include "NamingContextRoot.h"
#include "IteratorStack.h"

namespace CosNaming {
namespace Core {

class NIRVANA_NOVTABLE NamingContextBase : public NamingContextRoot
{
public:
	void bind (Name& n, CORBA::Object::_ptr_type obj);
	virtual void bind1 (Name& n, CORBA::Object::_ptr_type obj) = 0;

	void rebind (Name& n, CORBA::Object::_ptr_type obj);
	virtual void rebind1 (Name& n, CORBA::Object::_ptr_type obj) = 0;

	void bind_context (Name& n, NamingContext::_ptr_type nc);
	virtual void bind_context1 (Name& n, NamingContext::_ptr_type nc) = 0;

	void rebind_context (Name& n, NamingContext::_ptr_type nc);
	virtual void rebind_context1 (Name& n, NamingContext::_ptr_type nc) = 0;

	CORBA::Object::_ref_type resolve (Name& n);
	virtual CORBA::Object::_ref_type resolve1 (Name& n) = 0;

	void unbind (Name& n);
	virtual void unbind1 (Name& n) = 0;

	NamingContext::_ref_type bind_new_context (Name& n);
	virtual NamingContext::_ref_type bind_new_context1 (Name& n) = 0;

	void list (uint32_t how_many, BindingList& bl, CosNaming::BindingIterator::_ref_type& bi) const
	{
		check_exist ();
		NamingContextRoot::list (how_many, bl, bi);
	}

	void destroy ()
	{
		check_exist ();
		destroyed_ = true;
	}

	bool destroyed () const noexcept
	{
		return destroyed_;
	}

	void check_exist () const;
	void check_name (const Name& n) const;

protected:
	NamingContextBase (uint32_t signature = 0) :
		NamingContextRoot (signature),
		destroyed_ (false)
	{}

	virtual std::unique_ptr <Iterator> make_iterator () const override;
	virtual void get_bindings (IteratorStack& iter) const = 0;

private:
	NamingContext::_ref_type resolve_child (Name& n);

private:
	bool destroyed_;
};

}
}

#endif
