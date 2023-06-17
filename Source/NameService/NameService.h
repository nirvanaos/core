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
#ifndef NIRVANA_NAMESERVICE_NAMESERVICE_H_
#define NIRVANA_NAMESERVICE_NAMESERVICE_H_
#pragma once

#include "NamingContextImpl.h"
#include "ORB/SysServant.h"
#include "ORB/system_services.h"
#include "FileSystem.h"

namespace CosNaming {
namespace Core {

/// Naming Service root
class NameService : 
	public CORBA::Core::SysServantImpl <NameService, NamingContextExt, NamingContext>,
	public NamingContextImpl
{
	typedef NamingContextImpl Base;

public:
	// NamingContext

	virtual void bind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override
	{
		if (file_system_.find (name))
			throw NamingContext::AlreadyBound ();

		Base::bind1 (std::move (name), obj, n);
	}

	virtual void rebind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n) override
	{
		if (file_system_.find (name))
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_object, std::move (n));

		return Base::rebind1 (std::move (name), obj, n);
	}

	virtual void bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override
	{
		if (file_system_.find (name))
			throw NamingContext::AlreadyBound ();

		Base::bind_context1 (std::move (name), nc, n);
	}

	virtual void rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n) override
	{
		if (file_system_.find (name))
			throw NamingContext::CannotProceed (_this (), std::move (n));

		Base::rebind_context1 (std::move (name), nc, n);
	}

	virtual CORBA::Object::_ref_type resolve1 (const Istring& name, BindingType& type, Name& n) override
	{
		CORBA::Object::_ref_type obj = file_system_.resolve (name);
		if (obj)
			type = BindingType::ncontext;
		else
			obj = Base::resolve1 (name, type, n);
		return obj;
	}

	void unbind (Name& n)
	{
		Base::unbind (n);
	}

	NamingContext::_ref_type bind_new_context (Name& n)
	{
		return Base::bind_new_context (n);
	}

	static void destroy ()
	{
		throw NotEmpty ();
	}

	virtual std::unique_ptr <StackIterator> make_iterator () const
	{
		std::unique_ptr <StackIterator> iter (std::make_unique <StackIterator> ());
		iter->reserve (bindings_.size () + file_system_.root_cnt ());
		get_bindings (*iter);
		file_system_.get_roots (*iter);
		return iter;
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

private:
	Nirvana::FS::Core::FileSystem file_system_;
};

}
}

#endif
