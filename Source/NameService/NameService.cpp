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
#include "NameService.h"
#include "../ORB/ESIOP.h"
#include "../ORB/ORB.h"
#include "../ORB/ServantProxyObject.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

Object::_ref_type create_NameService ()
{
	if (ESIOP::is_system_domain ())
		return make_reference <NameService> ()->_this ();
	else
		return CORBA::Core::ORB::string_to_object (
			"corbaloc::1.1@/NameService", CORBA::Internal::RepIdOf <CosNaming::NamingContextExt>::id);
}

void NameService::bind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	if (file_system_.find (name))
		throw NamingContext::AlreadyBound ();

	Base::bind1 (std::move (name), obj, n);
}

void NameService::rebind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	if (file_system_.find (name))
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_object, std::move (n));

	return Base::rebind1 (std::move (name), obj, n);
}

void NameService::bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	if (file_system_.find (name))
		throw NamingContext::AlreadyBound ();

	Base::bind_context1 (std::move (name), nc, n);
}

void NameService::rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	if (file_system_.find (name))
		throw NamingContext::CannotProceed (_this (), std::move (n));

	Base::rebind_context1 (std::move (name), nc, n);
}

Object::_ref_type NameService::resolve1 (const Istring& name, BindingType& type, Name& n)
{
	Object::_ref_type obj = file_system_.resolve_root (name);
	if (obj)
		type = BindingType::ncontext;
	else
		obj = Base::resolve1 (name, type, n);
	return obj;
}

NamingContext::_ref_type NameService::create_context1 (Istring&& name, Name& n, bool& created)
{
	NamingContext::_ref_type nc = file_system_.resolve_root (name);
	if (nc)
		created = false;
	else
		nc = Base::create_context1 (std::move (name), n, created);
	return nc;
}

NamingContext::_ref_type NameService::bind_new_context1 (Istring&& name, Name& n)
{
	if (file_system_.find (name))
		throw NamingContext::AlreadyBound ();
	else
		return Base::bind_new_context1 (std::move (name), n);
}

std::unique_ptr <Iterator> NameService::make_iterator () const
{
	auto iter (std::make_unique <IteratorStack> ());
	iter->reserve (bindings_.size () + file_system_.root_cnt ());
	get_bindings (*iter);
	file_system_.get_roots (*iter);
	return iter;
}

}
}

