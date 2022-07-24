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
#include "POA_Base.h"

using namespace CORBA;
using namespace CORBA::Internal;

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

void POA_Base::_s_set_servant (CORBA::Internal::Bridge <POA>* _b,
	Interface* p_servant,
	Interface* _env)
{
	try {
		_implementation (_b).set_servant (CORBA::Internal::Type <CORBA::Object>::in (p_servant));
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

Object::_ref_type POA_Base::reference_to_servant (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	auto servant = object2servant_core (reference);
	POA::_ptr_type activated_POA = servant->activated_POA ();
	if (!activated_POA)
		throw ObjectNotActive ();
	if (!activated_POA->_is_equivalent (_this ()))
		throw WrongAdapter ();
	return reference;
}

ObjectId POA_Base::reference_to_id (Object::_ptr_type reference)
{
	auto servant = object2servant_core (reference);
	POA::_ptr_type activated_POA = servant->activated_POA ();
	if (!activated_POA || !activated_POA->_is_equivalent (_this ()))
		throw WrongAdapter ();
	return *servant->activated_id ();
}

}
}
