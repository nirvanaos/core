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
#include "NamingContextBase.h"
#include "Iterator.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

Istring NamingContextBase::to_string (const NameComponent& nc)
{
	Istring name = nc.id ();
	name += '.';
	name += nc.kind ();
	return name;
}

void NamingContextBase::check_name (const Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
}

void NamingContextBase::bind (Name& n, Object::_ptr_type obj)
{
	check_name (n);

	if (n.size () == 1)
		bind1 (to_string (n.front ()), obj, n);
	else
		resolve_context (n)->bind (n, obj);
}

void NamingContextBase::rebind (Name& n, Object::_ptr_type obj)
{
	check_name (n);

	if (n.size () == 1)
		rebind1 (to_string (n.front ()), obj, n);
	else
		resolve_context (n)->rebind (n, obj);
}

void NamingContextBase::bind_context (Name& n, NamingContext::_ptr_type nc)
{
	check_name (n);

	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1)
		bind_context1 (to_string (n.front ()), nc, n);
	else {
		Name created;
		try {
			create_context (n, created)->bind_context (n, nc);
		} catch (...) {
			unbind_created (created);
			throw;
		}
	}
}

void NamingContextBase::rebind_context (Name& n, NamingContext::_ptr_type nc)
{
	check_name (n);

	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1)
		rebind_context1 (to_string (n.front ()), nc, n);
	else {
		Name created;
		try {
			create_context (n, created)->rebind_context (n, nc);
		} catch (...) {
			unbind_created (created);
			throw;
		}
	}
}

NamingContext::_ref_type NamingContextBase::bind_new_context (Name& n)
{
	check_name (n);

	if (n.size () == 1)
		return bind_new_context1 (to_string (n.front ()), n);
	else {
		Name created;
		try {
			return create_context (n, created)->bind_new_context (n);
		} catch (...) {
			unbind_created (created);
			throw;
		}
	}
}

NamingContext::_ref_type NamingContextBase::create_context (Name& n, Name& created_name)
{
	created_name.clear ();
	bool created;
	NamingContext::_ref_type nc = create_context1 (to_string (n.front ()), n, created);
	if (created)
		created_name.push_back (std::move (n.front ()));
	n.erase (n.begin ());
	return nc;
}

void NamingContextBase::unbind_created (Name& created) noexcept
{
	if (!created.empty ()) {
		try {
			unbind (created);
		} catch (...) {}
	}
}

NamingContext::_ref_type NamingContextBase::resolve_context (Name& n)
{
	BindingType type;
	Object::_ref_type obj = resolve1 (to_string (n.front ()), type, n);
	if (type != BindingType::ncontext)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, n);

	n.erase (n.begin ());
	if (n.size () > 1) {
		Name cn;
		size_t cn_size = n.size () - 1;
		cn.reserve (cn_size);
		Name::iterator cn_end = n.begin () + cn_size;
		Name::iterator ni = n.begin ();
		do {
			cn.push_back (std::move (*(ni++)));
		} while (cn_end != ni);
		n.erase (n.begin (), cn_end);
		NamingContext::_ref_type child = NamingContext::_narrow (obj);
		assert (child);
		obj = child->resolve (cn);
		if (!obj)
			throw NamingContext::CannotProceed (child, cn);
		child = NamingContext::_narrow (obj);
		if (!child) {
			cn.erase (cn.begin (), cn.begin () + (cn.size () - 1));
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (cn));
		}
		return child;
	} else
		return NamingContext::_narrow (obj);
}

Object::_ref_type NamingContextBase::resolve (Name& n)
{
	check_name (n);

	BindingType type;
	Object::_ref_type obj = resolve1 (to_string (n.front ()), type, n);
	if (n.size () == 1)
		return obj;
	else if (type != BindingType::ncontext)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
	else {
		n.erase (n.begin ());
		return NamingContext::_narrow (obj)->resolve (n);
	}
}

void NamingContextBase::unbind (Name& n)
{
	check_name (n);

	if (n.size () == 1)
		unbind1 (to_string (n.front ()), n);
	else
		resolve_context (n)->unbind (n);
}

void NamingContextBase::list (uint32_t how_many, BindingList& bl,
	CosNaming::BindingIterator::_ref_type& bi) const
{
	auto vi = make_iterator ();
	vi->next_n (how_many, bl);
	if (!vi->end ())
		bi = Iterator::create_iterator (std::move (vi));
}

}
}
