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
#include "../ORB/ServantProxyObject.h"
#include "../deactivate_servant.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

/// Default implementation of the NamingContex interface.
class NamingContextDefault :
	public servant_traits <NamingContext>::Servant <NamingContextDefault>,
	public NamingContextImpl
{
public:
	bool _non_existent () const
	{
		return destroyed ();
	}

	void destroy ()
	{
		NamingContextImpl::destroy ();
		Nirvana::Core::deactivate_servant (this);
	}

	virtual NamingContext::_ptr_type this_context () override
	{
		return _this ();
	}

	virtual void add_link (const NamingContextImpl& parent) override;
	virtual bool remove_link (const NamingContextImpl& parent) override;
	virtual bool is_cyclic (ContextSet& parents) const override;

private:
	ContextSet links_;
};

void NamingContextDefault::add_link (const NamingContextImpl& parent)
{
	links_.insert (&parent);
}

bool NamingContextDefault::remove_link (const NamingContextImpl& parent)
{
	if (links_.size () > 1) {
		bool garbage = true;
		for (auto p : links_) {
			ContextSet parents;
			parents.insert (this);
			if (p != &parent && !p->is_cyclic (parents)) {
				garbage = false;
				break;
			}
		}
		if (garbage)
			return false;
	}
	links_.erase (&parent);
	return true;
}

bool NamingContextDefault::is_cyclic (ContextSet& parents) const
{
	if (links_.empty ())
		return false;
	if (!parents.insert (this).second)
		return true;
	for (auto p : links_) {
		if (!p->is_cyclic (parents))
			return false;
	}
	return true;
}

NamingContextImpl* NamingContextImpl::cast (Object::_ptr_type obj) noexcept
{
	NamingContextImpl* impl = static_cast <NamingContextImpl*> (
		static_cast <NamingContextDefault*> (CORBA::Core::object2servant_base (obj)));

	if (impl && impl->signature () == SIGNATURE)
		return impl;
	else
		return nullptr;
}

NamingContextImpl::NamingContextImpl () :
	NamingContextBase (SIGNATURE)
{}

NamingContextImpl::~NamingContextImpl ()
{}

void NamingContextImpl::shutdown () noexcept
{
	Bindings bindings (std::move (bindings_));
	while (!bindings.empty ()) {
		auto b = bindings.begin ();
		if (b->second.binding_type == BindingType::ncontext) {
			NamingContextImpl* impl = cast (b->second.object);
			if (impl)
				impl->shutdown ();
		}
		bindings.erase (b);
	}
}

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
	link (ins.first);
}

void NamingContextImpl::rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (nc, BindingType::ncontext));
	if (!ins.second) {
		if (ins.first->second.binding_type != BindingType::ncontext)
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
		else {
			Object::_ref_type old (std::move (ins.first->second.object));
			link (ins.first->second.object = nc);
			try {
				unlink (old, n);
			} catch (...) {
				unlink (ins.first->second.object, n);
				ins.first->second.object = std::move (old);
				throw;
			}
		}
	}
}

void NamingContextImpl::link (Bindings::iterator it)
{
	assert (BindingType::ncontext == it->second.binding_type);
	try {
		link (it->second.object);
	} catch (...) {
		bindings_.erase (it);
		throw;
	}
}

void NamingContextImpl::link (Object::_ptr_type context) const
{
	NamingContextImpl* impl = cast (context);
	if (impl)
		impl->add_link (*this);
}

void NamingContextImpl::unlink (Object::_ptr_type context, const Name& n)
{
	NamingContextImpl* impl = cast (context);
	if (impl && !impl->remove_link (*this))
		throw NamingContext::CannotProceed (this_context (), n);
}

NamingContext::_ref_type NamingContextImpl::create_context1 (Istring&& name, Name& n, bool& created)
{
	auto ins = bindings_.emplace (std::move (name), MapVal (Object::_nil (), BindingType::ncontext));
	if ((created = ins.second)) {
		try {
			link (ins.first->second.object = new_context ());
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
	if (BindingType::ncontext == it->second.binding_type)
		unlink (it->second.object, n);
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
		link (ins.first);
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
