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
#include "POAManagerFactory.h"

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

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
	parent_ (parent),
	name_ (name),
	the_POAManager_ (std::move (manager)),
	request_cnt_ (0),
	signature_ (SIGNATURE),
	destroyed_ (false)
{
	the_POAManager_->on_adapter_create (*this);
}

POA_Base::~POA_Base ()
{
	the_POAManager_->on_adapter_destroy (*this);
}

Object::_ref_type POA_Base::servant_to_reference (Object::_ptr_type p_servant)
{
	Object::_ref_type ret = servant_to_reference_nothrow (p_servant);
	if (!ret)
		throw WrongPolicy ();
	return ret;
}

Object::_ref_type POA_Base::servant_to_reference_nothrow (Object::_ptr_type p_servant)
{
	const Context* ctx = (const Context*)Nirvana::Core::TLS::current ().get (Nirvana::Core::TLS::CORE_TLS_PORTABLE_SERVER);
	if (ctx && (&ctx->object == &p_servant))
		return p_servant;
	return Object::_nil ();
}

Object::_ref_type POA_Base::reference_to_servant (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	auto proxy = object2proxy (reference);
	ObjectKeyRef key = proxy->object_key ();
	if (!key)
		throw ObjectNotActive ();
	if (!check_path (key->adapter_path ()))
		throw WrongAdapter ();
	return reference;
}

ObjectId POA_Base::reference_to_id (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	auto proxy = object2proxy (reference);
	ObjectKeyRef key = proxy->object_key ();
	if (!key)
		throw WrongAdapter ();
	if (!check_path (key->adapter_path ()))
		throw WrongAdapter ();
	return key->object_id ();
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

POA_Base& POA_Base::find_child (const IDL::String& adapter_name, bool activate_it)
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
	if (it == children_.end ())
		throw AdapterNonExistent ();
	else
		return *it->second;
}

void POA_Base::destroy (bool etherealize_objects, bool wait_for_completion)
{
	// Currently we do not support wait_for_completion.
	if (wait_for_completion)
		throw BAD_INV_ORDER (MAKE_OMG_MINOR (3));

	destroyed_ = true;
	Children tmp (std::move (children_));
	while (!tmp.empty ()) {
		tmp.begin ()->second->destroy (etherealize_objects, wait_for_completion);
	}
	if (parent_) {
		parent_->children_.erase (*name_);
		parent_ = nullptr;
	}
}

ObjectId POA_Base::servant_to_id (Object::_ptr_type p_servant)
{
	throw WrongPolicy ();
}

void POA_Base::serve (const RequestRef& request, ProxyObject& proxy)
{
	IOReference::OperationIndex op = proxy.find_operation (request->operation ());
	TLS& tls = TLS::current ();
	Context* ctx_prev = (Context*)tls.get (TLS::CORE_TLS_PORTABLE_SERVER);
	Context ctx (_this (), request->object_key ().object_id (), proxy);
	tls.set (TLS::CORE_TLS_PORTABLE_SERVER, &ctx);
	++request_cnt_;
	try {
		request->serve_request (proxy, op, request.memory ());
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
	--request_cnt_;
	the_POAManager_->on_request_finish ();
}

void RequestRef::reset () NIRVANA_NOEXCEPT
{
	ExecDomain& ed = ExecDomain::current ();
	try {
		ed.mem_context_push (memory_);
		{
			Base::reset ();
		}
		ed.mem_context_pop ();
	} catch (...) {}
	memory_.reset ();
}

}
}
