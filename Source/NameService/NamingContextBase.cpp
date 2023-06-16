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
#include "NamingContextBase.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

/// Default implementation of the NamingContex interface.
class NamingContextDefault :
	public servant_traits <NamingContext>::Servant <NamingContextDefault>,
	public NamingContextBase
{};

Istring NamingContextBase::to_string (const NameComponent& nc)
{
	Istring name = nc.id ();
	name += '.';
	name += nc.kind ();
	return name;
}

NameComponent NamingContextBase::to_component (const Istring& s)
{
	NameComponent nc;
	size_t dot = s.rfind ('.');
	if (dot == Istring::npos)
		nc.id (s);
	else {
		nc.id (s.substr (0, dot));
		nc.kind (s.substr (dot + 1));
	}
	return nc;
}

void NamingContextBase::bind (Name& n, Object::_ptr_type obj)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();

	if (n.size () == 1) {
		auto ins = bindings_.emplace (to_string (n.front ()), MapVal (obj, BindingType::nobject));
		if (!ins.second)
			throw NamingContext::AlreadyBound ();
	} else
		resolve_context (n)->bind (n, obj);
}

void NamingContextBase::rebind (Name& n, Object::_ptr_type obj)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();

	if (n.size () == 1) {
		auto ins = bindings_.emplace (to_string (n.front ()), MapVal (obj, BindingType::nobject));
		if (!ins.second) {
			if (ins.first->second.binding_type != BindingType::nobject)
				throw NamingContext::NotFound (NamingContext::NotFoundReason::not_object, n);
			else
				ins.first->second.object = obj;
		}
	} else
		resolve_context (n)->rebind (n, obj);
}

void NamingContextBase::bind_context (Name& n, NamingContext::_ptr_type nc)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1) {
		auto ins = bindings_.emplace (to_string (n.front ()), MapVal (nc, BindingType::ncontext));
		if (!ins.second)
			throw NamingContext::AlreadyBound ();
	} else {
		Istring created;
		try {
			create_context (n, created)->bind_context (n, nc);
		} catch (...) {
			if (!created.empty ())
				bindings_.erase (created);
			throw;
		}
	}
}

void NamingContextBase::rebind_context (Name& n, NamingContext::_ptr_type nc)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1) {
		auto ins = bindings_.emplace (to_string (n.front ()), MapVal (nc, BindingType::ncontext));
		if (!ins.second) {
			if (ins.first->second.binding_type != BindingType::ncontext)
				throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, n);
			else
				ins.first->second.object = nc;
		}
	} else {
		Istring created;
		try {
			create_context (n, created)->rebind_context (n, nc);
		} catch (...) {
			if (!created.empty ())
				bindings_.erase (created);
			throw;
		}
	}
}

NamingContext::_ref_type NamingContextBase::resolve_context (Name& n) const
{
	auto it = bindings_.find (to_string (n.front ()));
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, n);
	else if (it->second.binding_type != BindingType::ncontext)
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
		NamingContext::_ref_type child = NamingContext::_narrow (it->second.object);
		assert (child);
		Object::_ref_type obj = child->resolve (cn);
		if (!obj)
			throw NamingContext::CannotProceed (child, cn);
		child = NamingContext::_narrow (obj);
		if (!child) {
			cn.erase (cn.begin (), cn.begin () + (cn.size () - 1));
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, cn);
		}
		return child;
	} else
		return NamingContext::_narrow (it->second.object);
}

NamingContext::_ref_type NamingContextBase::create_context (Name& n, Istring& created)
{
	auto ins = bindings_.emplace (to_string (n.front ()), MapVal (Object::_nil (), BindingType::ncontext));
	if (ins.second) {
		try {
			ins.first->second.object = new_context ();
			created = to_string (n.front ());
		} catch (...) {
			bindings_.erase (ins.first);
			throw;
		}
	} else if (ins.first->second.binding_type != BindingType::ncontext)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, n);
	n.erase (n.begin ());
	return NamingContext::_narrow (ins.first->second.object);
}

Object::_ref_type NamingContextBase::resolve (Name& n) const
{
	if (n.empty ())
		throw NamingContext::InvalidName ();

	auto it = bindings_.find (to_string (n.front ()));
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, n);
	if (n.size () == 1)
		return it->second.object;
	else if (it->second.binding_type != BindingType::ncontext)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, n);
	else {
		n.erase (n.begin ());
		return NamingContext::_narrow (it->second.object)->resolve (n);
	}
}

void NamingContextBase::unbind (Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();

	if (n.size () == 1) {
		auto it = bindings_.find (to_string (n.front ()));
		if (it == bindings_.end ())
			throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, n);
		bindings_.erase (it);
	} else
		resolve_context (n)->unbind (n);
}

NamingContext::_ref_type NamingContextBase::new_context ()
{
	return CORBA::make_reference <NamingContextDefault> ()->_this ();
}

NamingContext::_ref_type NamingContextBase::bind_new_context (Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();

	if (n.size () == 1) {
		auto ins = bindings_.emplace (to_string (n.front ()), MapVal (Object::_nil (), BindingType::ncontext));
		if (ins.second) {
			try {
				ins.first->second.object = new_context ();
			} catch (...) {
				bindings_.erase (ins.first);
				throw;
			}
			return NamingContext::_narrow (ins.first->second.object);
		} else
			throw NamingContext::AlreadyBound ();
	} else {
		Istring created;
		try {
			return create_context (n, created)->bind_new_context (n);
		} catch (...) {
			if (!created.empty ())
				bindings_.erase (created);
			throw;
		}
	}
}

void NamingContextBase::destroy ()
{
	if (!bindings_.empty ())
		throw NamingContext::NotEmpty ();
}

void NamingContextBase::list (uint32_t how_many, BindingList& bl, BindingIterator::_ref_type& bi)
{
	throw CORBA::NO_IMPLEMENT ();
}

}
}
