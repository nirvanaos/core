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
#include "../pch.h"
#include "NamingContextImpl.h"
#include "NamingContextDefault.h"
#include "NameService.h"
#include "../ORB/ReferenceLocal.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

NamingContextImpl* NamingContextImpl::cast (Object::_ptr_type obj) noexcept
{
	using namespace CORBA::Core;

	if (!obj)
		return nullptr;

	NamingContextImpl* impl = nullptr;

	Reference* ref = ProxyManager::cast (obj)->to_reference ();
	if (ref) {
		// Reference
		if (!(ref->flags () & Reference::LOCAL))
			return nullptr; // Remote reference

		servant_reference <ServantProxyObject> proxy;
		proxy = static_cast <ReferenceLocal*> (ref)->get_active_servant_with_lock ();
		if (!proxy)
			return nullptr; // Inactive reference

		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			return nullptr; // Other sync context

		impl = static_cast <NamingContextDefault*> (proxy2servant_base (proxy));
	} else {
		// Servant proxy
		const ServantProxyBase* proxy = object2proxy_base (obj);
		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			return nullptr; // Other sync context

		CORBA::Internal::Interface::_ptr_type servant = proxy->servant ();
		if (CORBA::Internal::RepId::compatible (servant->_epv ().interface_id,
			CORBA::Internal::RepIdOf <PortableServer::ServantBase>::id)
			) {
			impl = static_cast <NamingContextDefault*> (
				static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>*> (&servant));
		} else {
			assert (CORBA::Internal::RepId::compatible (servant->_epv ().interface_id,
				CORBA::Internal::RepIdOf <CORBA::LocalObject>::id));
			impl = static_cast <NameService*> (
				static_cast <CORBA::Internal::Bridge <CORBA::LocalObject>*> (&servant));
		}
	}

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

void NamingContextImpl::terminate () noexcept
{
	Bindings bindings (std::move (bindings_));
	while (!bindings.empty ()) {
		auto b = bindings.begin ();
		if (b->second.binding_type == BindingType::ncontext) {
			NamingContextImpl* impl = cast (b->second.object);
			if (impl)
				impl->terminate ();
		}
		bindings.erase (b);
	}
}

std::pair <NamingContextImpl::Bindings::iterator, bool> NamingContextImpl::emplace (const Name& n,
	Object::_ptr_type obj, BindingType type)
{
	assert (!n.empty ());
	return bindings_.emplace (std::piecewise_construct,
		std::forward_as_tuple (to_string (n.front ())),
		std::forward_as_tuple (obj, type));
}

void NamingContextImpl::bind1 (Name& n, Object::_ptr_type obj)
{
	assert (n.size () == 1);
	auto ins = emplace (n, obj, BindingType::nobject);

	if (!ins.second)
		throw NamingContext::AlreadyBound ();
}

void NamingContextImpl::rebind1 (Name& n, CORBA::Object::_ptr_type obj)
{
	assert (n.size () == 1);
	auto ins = emplace (n, obj, BindingType::nobject);

	if (!ins.second) {
		if (ins.first->second.binding_type != BindingType::nobject)
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_object, std::move (n));
		else
			ins.first->second.object = obj;
	}
}

void NamingContextImpl::bind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	assert (n.size () == 1);
	assert (nc);
	auto ins = emplace (n, nc, BindingType::ncontext);

	if (!ins.second)
		throw NamingContext::AlreadyBound ();
	try {
		link (nc);
	} catch (...) {
		bindings_.erase (ins.first);
		throw;
	}
}

void NamingContextImpl::rebind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	assert (n.size () == 1);
	assert (nc);
	auto ins = emplace (n, nc, BindingType::ncontext);

	if (!ins.second) {
		if (ins.first->second.binding_type != BindingType::ncontext)
			throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
		else {
			NamingContextImpl* impl_new = cast (nc);
			if (impl_new)
				impl_new->add_link (*this);
			Object::_ref_type old (std::move (ins.first->second.object));
			ins.first->second.object = nc;
			NamingContextImpl* impl_old = cast (old);
			if (impl_old) {
				try {
					unlink (*impl_old, n);
				} catch (...) {
					if (impl_new)
						unlink (*impl_new, n);
					ins.first->second.object = std::move (old);
					throw;
				}
			}
		}
	}
}

void NamingContextImpl::link (NamingContext::_ptr_type context) const
{
	NamingContextImpl* impl = cast (context);
	if (impl)
		link (*impl);
}

void NamingContextImpl::unlink (Object::_ptr_type context, const Name& n)
{
	NamingContextImpl* impl = cast (context);
	if (impl)
		unlink (*impl, n);
}

void NamingContextImpl::unlink (NamingContextImpl& context, const Name& n)
{
	if (!context.remove_link (*this))
		throw NamingContext::CannotProceed (this_context (), n);
}

Object::_ref_type NamingContextImpl::resolve1 (Name& n)
{
	assert (!n.empty ());
	auto it = bindings_.find (to_string (n.front ()));
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));
	return it->second.object;
}

void NamingContextImpl::unbind1 (Name& n)
{
	assert (!n.empty ());
	auto it = bindings_.find (to_string (n.front ()));
	if (it == bindings_.end ())
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));
	if (BindingType::ncontext == it->second.binding_type)
		unlink (it->second.object, n);
	bindings_.erase (it);
}

NamingContext::_ref_type NamingContextImpl::bind_new_context1 (Name& n)
{
	assert (n.size () == 1);
	servant_reference <NamingContextDefault> servant = NamingContextDefault::create ();
	NamingContext::_ref_type nc = servant->_this ();
	auto ins = emplace (n, nc, BindingType::ncontext);
	if (ins.second) {
		try {
			link (*servant);
		} catch (...) {
			bindings_.erase (ins.first);
			throw;
		}
		return nc;
	} else
		throw NamingContext::AlreadyBound ();
}

void NamingContextImpl::get_bindings (IteratorStack& iter) const
{
	iter.reserve (bindings_.size ());
	for (const auto& b : bindings_) {
		iter.push (to_component (b.first), b.second.binding_type);
	}
}

}
}
