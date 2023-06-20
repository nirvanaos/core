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
#ifndef NIRVANA_NAMESERVICE_NAMINGCONTEXTIMPL_H_
#define NIRVANA_NAMESERVICE_NAMINGCONTEXTIMPL_H_
#pragma once

#include "NamingContextBase.h"
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include "../PointerSet.h"

namespace CosNaming {
namespace Core {

class IteratorStack;

class NIRVANA_NOVTABLE NamingContextImpl : public NamingContextBase
{
public:
	virtual void bind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override;
	virtual void rebind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override;
	virtual void bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override;
	virtual void rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override;
	virtual CORBA::Object::_ref_type resolve1 (const Istring& name, BindingType& type, Name& n) override;
	virtual void unbind1 (const Istring& name, Name& n) override;
	virtual NamingContext::_ptr_type this_context () = 0;

	NamingContext::_ref_type new_context ();

	virtual NamingContext::_ref_type bind_new_context1 (Istring&& name, Name& n) override;

	void destroy ()
	{
		if (!bindings_.empty ())
			throw NamingContext::NotEmpty ();
	}

	virtual std::unique_ptr <Iterator> make_iterator () const override;

	static NamingContextImpl* cast (CORBA::Object::_ptr_type obj) noexcept;

	virtual void add_link (const NamingContextImpl& parent) = 0;
	virtual bool remove_link (const NamingContextImpl& parent) = 0;

	typedef Nirvana::Core::PointerSet <const NamingContextImpl> ContextSet;

	virtual bool is_cyclic (ContextSet& parents) const = 0;

	void shutdown () noexcept;

protected:
	static const uint32_t SIGNATURE = 'NAME';

	NamingContextImpl ();
	~NamingContextImpl ();

	void get_bindings (IteratorStack& iter) const;
	virtual NamingContext::_ref_type create_context1 (Istring&& name, Name& n, bool& created) override;

private:
	struct MapVal
	{
		CORBA::Object::_ref_type object;
		BindingType binding_type;

		MapVal (CORBA::Object::_ptr_type obj, BindingType type) :
			object (obj),
			binding_type (type)
		{}

		MapVal () :
			binding_type (BindingType::nobject)
		{}
	};

	using Bindings = Nirvana::Core::MapUnorderedUnstable <Istring, MapVal,
		std::hash <Istring>, std::equal_to <Istring>, Nirvana::Core::UserAllocator>;

	void link (CORBA::Object::_ptr_type context) const;
	void link (Bindings::iterator it);
	void unlink (CORBA::Object::_ptr_type context, const Name& n);

protected:
	Bindings bindings_;
};

}
}

#endif
