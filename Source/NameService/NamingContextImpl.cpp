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

NamingContextImpl::NamingContextImpl ()
{}

NamingContextImpl::~NamingContextImpl ()
{}

void NamingContextImpl::bind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (obj, BindingType::nobject));
	if (!ins.second)
		throw NamingContext::AlreadyBound ();
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

void NamingContextImpl::bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (nc, BindingType::ncontext));
	if (!ins.second)
		throw NamingContext::AlreadyBound ();
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

NamingContext::_ref_type NamingContextImpl::create_context1 (Istring&& name, Name& n, bool& created)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (Object::_nil (), BindingType::ncontext));
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

Object::_ref_type NamingContextImpl::resolve1 (const Istring& name, BindingType& type, Name& n)
{
	auto it = bindings_.find (name);
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));
	type = it->second.binding_type;
	return it->second.object;
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
