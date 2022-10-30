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
#include "POA_Root.h"
#include "PortableServer_Context.h"
#include <CORBA/Servant_var.h>

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

POA_Root* POA_Base::root_ = nullptr;

Interface* POA_Base::_s_get_servant (Bridge <POA>* _b, Interface* _env)
{
	try {
		return Type <Object>::ret (_implementation (_b).get_servant ());
	} catch (Exception& e) {
		set_exception (_env, e);
	} catch (...) {
		set_unknown_exception (_env);
	}
	return Type <Object>::ret ();
}

void POA_Base::_s_set_servant (Bridge <POA>* _b,
	Interface* p_servant,
	Interface* _env)
{
	try {
		_implementation (_b).set_servant (Type <Object>::in (p_servant));
	} catch (Exception& e) {
		set_exception (_env, e);
	} catch (...) {
		set_unknown_exception (_env);
	}
}

Type <ObjectId>::ABI_ret POA_Base::_s_activate_object (Bridge <POA>* _b,
	Interface* p_servant,
	Interface* env)
{
	try {
		return Type <ObjectId>::ret (_implementation (_b).
			activate_object (Type <Object>::in (p_servant)));
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
	return Type <ObjectId>::ret ();
}

void POA_Base::_s_activate_object_with_id (Bridge <POA>* _b,
	Type <ObjectId>::ABI_in oid, Interface* p_servant,
	Interface* env)
{
	try {
		_implementation (_b).activate_object_with_id (Type <ObjectId>::in (oid), Type <Object>::in (p_servant));
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
}

Type <ObjectId>::ABI_ret POA_Base::_s_servant_to_id (Bridge <POA>* _b,
	Interface* p_servant,
	Interface* env)
{
	try {
		return Type <ObjectId>::ret (_implementation (_b).
			servant_to_id (Type <Object>::in (p_servant)));
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
	return Type <ObjectId>::ret ();
}

Interface* POA_Base::_s_servant_to_reference (Bridge <POA>* _b,
	Interface* p_servant,
	Interface* env)
{
	try {
		return Type <Object>::ret (_implementation (_b).
			servant_to_reference (Type <Object>::in (p_servant)));
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
	return Type <Object>::ret ();
}

Interface* POA_Base::_s_reference_to_servant (Bridge <POA>* _b,
	Interface* reference,
	Interface* env)
{
	try {
		return Type <Object>::ret (_implementation (_b).
			reference_to_servant (Type <Object>::in (reference)));
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
	return Type <Object>::ret ();
}

Interface* POA_Base::_s_id_to_servant (Bridge <POA>* _b,
	Type <ObjectId>::ABI_in oid,
	Interface* env)
{
	try {
		return Type <Object>::ret (_implementation (_b).
			id_to_reference (Type <ObjectId>::in (oid))); // Really call to id_to_reference()
	} catch (Exception& e) {
		set_exception (env, e);
	} catch (...) {
		set_unknown_exception (env);
	}
	return Type <Object>::ret ();
}

POA_Base::POA_Base (POA_Base* parent, const IDL::String* name,
	CORBA::servant_reference <POAManager>&& manager) :
	the_POAManager_ (std::move (manager)),
	parent_ (parent),
	name_ (name),
	request_cnt_ (0),
	destroyed_ (false),
	signature_ (SIGNATURE)
{
	the_POAManager_->on_adapter_create (*this);

#ifdef _DEBUG
	for (size_t i = 1; i < FACTORY_COUNT; ++i) {
		assert (factories_ [i - 1].policies < factories_ [i].policies);
	}
#endif
}

POA_Base::~POA_Base ()
{
	the_POAManager_->on_adapter_destroy (*this);
}

void POA_Base::activate_object (CORBA::Core::ProxyObject& proxy, ObjectId& oid, unsigned flags)
{
	for (;;) {
		oid = generate_object_id ();
		ReferenceLocalRef ref = activate_object (ObjectKey (*this, oid), true, proxy, 0);
		if (ref)
			return;
	}
}

ReferenceLocalRef POA_Base::activate_object (CORBA::Core::ProxyObject& proxy, unsigned flags)
{
	for (;;) {
		ReferenceLocalRef ref = activate_object (ObjectKey (*this), true, proxy, 0);
		if (ref)
			return ref;
	}
}

ReferenceLocalRef POA_Base::activate_object (ObjectKey&&, bool unique, ProxyObject&, unsigned flags)
{
	throw WrongPolicy ();
}

void POA_Base::activate_object (ReferenceLocal& ref, ProxyObject& proxy, unsigned flags)
{
	NIRVANA_UNREACHABLE_CODE ();
}

bool POA_Base::implicit_activation () const NIRVANA_NOEXCEPT
{
	return false;
}

void POA_Base::deactivate_object (const ObjectId& oid)
{
	throw WrongPolicy ();
}

servant_reference <CORBA::Core::ProxyObject> POA_Base::deactivate_object (ReferenceLocal& ref)
{
	NIRVANA_UNREACHABLE_CODE ();
	return nullptr;
}

void POA_Base::implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
	CORBA::Core::ProxyObject& proxy) NIRVANA_NOEXCEPT
{
	NIRVANA_UNREACHABLE_CODE ();
}

Object::_ref_type POA_Base::create_reference (const RepositoryId& intf)
{
	return create_reference (intf, 0)->get_proxy ();
}

ReferenceLocalRef POA_Base::create_reference (const RepositoryId& intf, unsigned flags)
{
	for (;;) {
		ReferenceLocalRef ref = root_->emplace_reference (ObjectKey (*this), true, std::ref (intf), flags);
		if (ref)
			return ref;
		assert (false); // Unique ID collision.
	}
}

ReferenceLocalRef POA_Base::create_reference (ObjectKey&& key,
	const CORBA::RepositoryId& intf)
{
	return create_reference (std::move (key), intf, 0);
}

ReferenceLocalRef POA_Base::create_reference (ObjectKey&& key, const RepositoryId& intf,
	unsigned flags)
{
	return root_->emplace_reference (std::move (key), false, std::ref (intf), flags);
}

ObjectId POA_Base::servant_to_id (ProxyObject& proxy)
{
	return servant_to_id_default (proxy, false);
}

ObjectId POA_Base::servant_to_id_default (ProxyObject& proxy, bool not_found)
{
	if (not_found)
		throw ServantNotActive ();
	else
		throw WrongPolicy ();
}

Object::_ref_type POA_Base::servant_to_reference (ProxyObject& proxy)
{
	return servant_to_reference_default (proxy, false);
}

Object::_ref_type POA_Base::servant_to_reference_default (ProxyObject& proxy, bool not_found)
{
	if (not_found) {
		const Context* ctx = (const Context*)Nirvana::Core::TLS::current ().get (
			Nirvana::Core::TLS::CORE_TLS_PORTABLE_SERVER);
		if (ctx && (&ctx->servant == &proxy.get_proxy ()))
			return ctx->servant;
		throw ServantNotActive ();
	} else
		throw WrongPolicy ();
}

Object::_ref_type POA_Base::reference_to_servant (Object::_ptr_type reference)
{
	return reference_to_servant_default (false);
}

Object::_ref_type POA_Base::reference_to_servant_default (bool not_active)
{
	if (not_active)
		throw ObjectNotActive ();
	else
		throw WrongPolicy ();
}

ObjectId POA_Base::reference_to_id (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	ReferenceRef ref = ProxyManager::cast (reference)->get_reference ();
	if (!ref)
		throw WrongAdapter ();
	if (!(ref->flags () & Reference::LOCAL))
		throw WrongAdapter ();
	const ReferenceLocal& loc = static_cast <const ReferenceLocal&> (*ref);
	if (!check_path (loc.object_key ().adapter_path ()))
		throw WrongAdapter ();
	return loc.object_key ().object_id ();
}

Object::_ref_type POA_Base::id_to_servant (const ObjectId& oid)
{
	return id_to_servant_default (false);
}

Object::_ref_type POA_Base::id_to_servant_default (bool not_active)
{
	if (not_active)
		throw ObjectNotActive ();
	else
		throw WrongPolicy ();
}

Object::_ref_type POA_Base::id_to_reference (const ObjectId& oid)
{
	throw WrongPolicy ();
}

POA_Base* POA_Base::get_implementation (const CORBA::Core::ProxyLocal* proxy)
{
	if (proxy) {
		POA_Base* impl = static_cast <POA_Base*> (
			static_cast <Bridge <CORBA::LocalObject>*> (&proxy->servant ()));
		if (impl->signature_ == SIGNATURE)
			return impl;
		else
			throw CORBA::OBJ_ADAPTER (); // User try to create own POA implementation?
	}
	return nullptr;
}

void POA_Base::get_path (AdapterPath& path, size_t size) const
{
	if (parent_) {
		parent_->get_path (path, size + 1);
		path.push_back (*name_);
	} else {
		path.clear ();
		path.reserve (size);
	}
}

bool POA_Base::check_path (const AdapterPath& path, AdapterPath::const_iterator it)
	const NIRVANA_NOEXCEPT
{
	if (parent_) {
		assert (name_);
		if (it == path.begin ())
			return false;
		--it;
		if (name_->size () != it->size ())
			return false;
		if (*name_ != *it)
			return false;
		return parent_->check_path (path, it);
	} else
		return it == path.begin ();
}

POA_Ref POA_Base::find_child (const IDL::String& adapter_name, bool activate_it)
{
	auto it = children_.find (adapter_name);
	if (it == children_.end ()) {
		if (activate_it && the_activator_) {
			if (is_destroyed ())
				throw TRANSIENT (MAKE_OMG_MINOR (4));

			bool created = false;
			try {
				created = the_activator_->unknown_adapter (_this (), adapter_name);
			} catch (const SystemException&) {
				throw OBJ_ADAPTER (MAKE_OMG_MINOR (1));
			}
			if (created)
				it = children_.find (static_cast <const IDL::String&> (adapter_name));
			else
				throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
		}
	}
	if (it != children_.end ())
		return Servant_var <POA_Base> (&*it->second);
	else
		return nullptr;
}

void POA_Base::destroy_internal (bool etherealize_objects) NIRVANA_NOEXCEPT
{
	destroyed_ = true;
	name_ = nullptr;
	Children tmp (std::move (children_));
	while (!tmp.empty ()) {
		tmp.begin ()->second->destroy (etherealize_objects);
		tmp.erase (tmp.begin ());
	}
}

ObjectId POA_Base::generate_object_id ()
{
	throw WrongPolicy ();
}

void POA_Base::check_object_id (const ObjectId& oid)
{}

void POA_Base::serve (const RequestRef& request)
{
	ReferenceLocalRef ref = root_->find_reference (request->object_key ());
	if (!ref)
		ref = create_reference (ObjectKey (request->object_key ()), IDL::String ());
	serve (request, *ref);
}

void POA_Base::serve (const RequestRef& request, ReferenceLocal& reference)
{
	serve_default (request, reference);
}

void POA_Base::serve_default (const RequestRef& request, ReferenceLocal& reference)
{
	throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
}

void POA_Base::serve (const RequestRef& request, ReferenceLocal& reference, ProxyObject& proxy)
{
	IOReference::OperationIndex op = proxy.find_operation (request->operation ());
	TLS& tls = TLS::current ();
	Context* ctx_prev = (Context*)tls.get (TLS::CORE_TLS_PORTABLE_SERVER);
	Context ctx (_this (), request->object_key ().object_id (), reference.get_proxy (), proxy);
	tls.set (TLS::CORE_TLS_PORTABLE_SERVER, &ctx);
	++request_cnt_;
	try {
		request->serve_request (proxy, op, request->memory ());
	} catch (...) {
		on_request_finish ();
		tls.set (TLS::CORE_TLS_PORTABLE_SERVER, ctx_prev);
		throw;
	}
	on_request_finish ();
	tls.set (TLS::CORE_TLS_PORTABLE_SERVER, ctx_prev);
}

void POA_Base::on_request_finish () NIRVANA_NOEXCEPT
{
	if (!--request_cnt_ && destroyed_)
		destroy_completed_.signal ();
	the_POAManager_->on_request_finish ();
}

void POA_Base::check_wait_completion ()
{
	// If wait_for_completion is TRUE and the current thread is in an invocation context dispatched
	// from some POA belonging to the same ORB as this POA, the BAD_INV_ORDER system exception with
	// standard minor code 3 is raised and POA destruction does not occur.
	if (Nirvana::Core::TLS::current ().get (Nirvana::Core::TLS::CORE_TLS_PORTABLE_SERVER))
		throw CORBA::BAD_INV_ORDER (MAKE_OMG_MINOR (3));
}

}
}
