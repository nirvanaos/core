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
#include "NamingContextImpl.h"
#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>
#include "IteratorStack.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

/// Default implementation of the NamingContex interface.
class NamingContextDefault :
	public servant_traits <NamingContext>::Servant <NamingContextDefault>,
	public NamingContextImpl
{
public:
	void destroy ()
	{
		NamingContextImpl::destroy ();
		_default_POA ()->deactivate_object (_default_POA ()->servant_to_id (this));
	}
};

Istring NamingContextImpl::to_string (const NameComponent& nc)
{
	Istring name = nc.id ();
	name += '.';
	name += nc.kind ();
	return name;
}

void NamingContextImpl::check_name (const Name& n)
{
	if (n.empty ())
		throw NamingContext::InvalidName ();
}

NamingContextImpl::NamingContextImpl ()
{}

NamingContextImpl::~NamingContextImpl ()
{}

void NamingContextImpl::bind (Name& n, Object::_ptr_type obj)
{
	check_name (n);

	if (n.size () == 1)
		bind1 (to_string (n.front ()), obj, n);
	else
		resolve_context (n)->bind (n, obj);
}

void NamingContextImpl::bind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (obj, BindingType::nobject));
	if (!ins.second)
		throw NamingContext::AlreadyBound ();
}

void NamingContextImpl::rebind (Name& n, Object::_ptr_type obj)
{
	check_name (n);

	if (n.size () == 1)
		rebind1 (to_string (n.front ()), obj, n);
	else
		resolve_context (n)->rebind (n, obj);
}

void NamingContextImpl::rebind1 (Istring&& name, CORBA::Object::_ptr_type obj, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (obj, BindingType::nobject));
	if (!ins.second) {
		if (ins.first->second.binding_type != BindingType::nobject)
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_object, std::move (n));
		else
			ins.first->second.object = obj;
	}
}

void NamingContextImpl::bind_context (Name& n, NamingContext::_ptr_type nc)
{
	check_name (n);

	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1)
		bind_context1 (to_string (n.front ()), nc, n);
	else {
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

void NamingContextImpl::bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (nc, BindingType::ncontext));
	if (!ins.second)
		throw NamingContext::AlreadyBound ();
}

void NamingContextImpl::rebind_context (Name& n, NamingContext::_ptr_type nc)
{
	check_name (n);

	if (!nc)
		throw BAD_PARAM ();

	if (n.size () == 1)
		rebind_context1 (to_string (n.front ()), nc, n);
	else {
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

void NamingContextImpl::rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (nc, BindingType::ncontext));
	if (!ins.second) {
		if (ins.first->second.binding_type != BindingType::ncontext)
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
		else
			ins.first->second.object = nc;
	}
}

NamingContext::_ref_type NamingContextImpl::create_context (Name& n, Istring& created_name)
{
	Istring name = to_string (n.front ());
	bool created;
	NamingContext::_ref_type nc = create_context1 (name, n, created);
	if (created)
		created_name = std::move (name);
	n.erase (n.begin ());
	return nc;
}

NamingContext::_ref_type NamingContextImpl::create_context1 (const Istring& name, Name& n, bool& created)
{
	auto ins = bindings_.emplace (name, MapVal (Object::_nil (), BindingType::ncontext));
	if ((created = ins.second)) {
		try {
			ins.first->second.object = new_context ();
		} catch (...) {
			bindings_.erase (ins.first);
			throw;
		}
	} else if (ins.first->second.binding_type != BindingType::ncontext)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));

	return NamingContext::_narrow (ins.first->second.object);
}

NamingContext::_ref_type NamingContextImpl::resolve_context (Name& n)
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

Object::_ref_type NamingContextImpl::resolve (Name& n)
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

Object::_ref_type NamingContextImpl::resolve1 (const Istring& name, BindingType& type, Name& n)
{
	auto it = bindings_.find (name);
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));
	type = it->second.binding_type;
	return it->second.object;
}

void NamingContextImpl::unbind (Name& n)
{
	check_name (n);

	if (n.size () == 1)
		unbind1 (to_string (n.front ()), n);
	else
		resolve_context (n)->unbind (n);
}

void NamingContextImpl::unbind1 (const Istring& name, Name& n)
{
	auto it = bindings_.find (name);
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));
	bindings_.erase (it);
}

NamingContext::_ref_type NamingContextImpl::new_context ()
{
	return CORBA::make_reference <NamingContextDefault> ()->_this ();
}

NamingContext::_ref_type NamingContextImpl::bind_new_context (Name& n)
{
	check_name (n);

	if (n.size () == 1)
		return bind_new_context1 (to_string (n.front ()), n);
	else {
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

NamingContext::_ref_type NamingContextImpl::bind_new_context1 (Istring&& name, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (Object::_nil (), BindingType::ncontext));
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
}

void NamingContextImpl::list (uint32_t how_many, BindingList& bl,
	CosNaming::BindingIterator::_ref_type& bi) const
{
	auto vi = make_iterator ();
	vi->next_n (how_many, bl);
	if (!vi->end ())
		bi = Iterator::create_iterator (std::move (vi));
}

std::unique_ptr <Iterator> NamingContextImpl::make_iterator () const
{
	std::unique_ptr <IteratorStack> iter (std::make_unique <IteratorStack> ());
	iter->reserve (bindings_.size ());
	get_bindings (*iter);
	return iter;
}

void NamingContextImpl::get_bindings (IteratorStack& iter) const
{
	for (const auto& b : bindings_) {
		iter.push (b.first, b.second.binding_type);
	}
}

}
}
