/// \file
/// Hand-made proxy code for the POA interface.
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
#include <CORBA/Proxy/Proxy.h>
#include <CORBA/POA_s.h>
#include <ORB/ServantBase.h>

NIRVANA_EXPORT (_exp_PortableServer_POA_ServantAlreadyActive, CORBA::Internal::RepIdOf <CORBA::Internal::Definitions <PortableServer::POA>::ServantAlreadyActive>::repository_id_,
CORBA::TypeCode, CORBA::Internal::TypeCodeException <CORBA::Internal::Definitions <PortableServer::POA>::ServantAlreadyActive, false>)

NIRVANA_EXPORT (_exp_PortableServer_POA_ObjectNotActive, CORBA::Internal::RepIdOf <CORBA::Internal::Definitions <PortableServer::POA>::ObjectNotActive>::repository_id_,
CORBA::TypeCode, CORBA::Internal::TypeCodeException <CORBA::Internal::Definitions <PortableServer::POA>::ObjectNotActive, false>)

namespace CORBA {
namespace Internal {

IMPLEMENT_PROXY_FACTORY (::PortableServer::, POA);

template <>
struct ProxyTraits <::PortableServer::POA>
{
	static const Operation operations_ [];
	static const Char* const interfaces_ [];

	// string activate_object (PortableServer::Servant p_servant);

	static const Parameter activate_object_in_params_ [];

	static void activate_object_request (::PortableServer::POA::_ptr_type _servant,
		IORequest::_ptr_type _call)
	{
		Object::_ref_type p_servant;
		Type <Object>::unmarshal (_call, p_servant);
		_call->unmarshal_end ();
		String ret;
		{
			Environment _env;
			Type <String>::C_ret _ret = (_servant->_epv ().epv.activate_object) (&_servant, &Object::_ptr_type (p_servant), &_env);
			_env.check ();
			ret = _ret;
		}
		// Marshal output
		Type <String>::marshal_out (ret, _call);
	}

	// void deactivate_object (string oid);

	static const Parameter deactivate_object_in_params_ [];

	static void deactivate_object_request (::PortableServer::POA_ptr _servant,
		IORequest::_ptr_type _call)
	{
		String oid;
		Type <String>::unmarshal (_call, oid);
		_call->unmarshal_end ();
		_servant->deactivate_object (oid);
	}
};

template <>
class Proxy <::PortableServer::POA> :
	public ProxyBase <::PortableServer::POA>
{
	typedef ProxyBase <::PortableServer::POA> Base;
	typedef ProxyTraits <::PortableServer::POA> Traits;
public:
	Proxy (IOReference::_ptr_type proxy_manager, CORBA::UShort interface_idx) :
		Base (proxy_manager, interface_idx)
	{}

	Object::_ptr_type servant2object (PortableServer::Servant servant)
	{
		PortableServer::Servant ps = servant->__core_servant ();
		Core::ServantBase* core_obj = static_cast <Core::ServantBase*> (&ps);
		return core_obj->get_proxy ();
	}

	// Returns nil for objects from other domains and for local objects
	PortableServer::ServantBase::_ref_type object2servant (Object::_ptr_type obj)
	{
		Environment _env;
		I_ret <PortableServer::ServantBase> _ret = (obj->_epv ().internal.get_servant) (&obj, &_env);
		_env.check ();
		return _ret;
	}

	String activate_object (PortableServer::Servant p_servant)
	{
		Object::_ptr_type proxy = servant2object (p_servant);
		IORequest::_ref_type _call = _target ()->create_request ();
		Type <Object>::marshal_in (proxy, _call);
		_call->issue (_make_op_idx (0), 0);
		check_request (_call);
		String _ret;
		Type <String>::unmarshal (_call, _ret);
		return _ret;
	}

	void deactivate_object (const String& oid)
	{
		IORequest::_ref_type _call = _target ()->create_request ();
		Type <String>::marshal_in (oid, _call);
		_call->issue (_make_op_idx (1), 0);
		check_request (_call);
	}
};

const Parameter ProxyTraits <::PortableServer::POA>::activate_object_in_params_ [1] = {
	{ "p_servant", Type <Object>::type_code }
};

const Parameter ProxyTraits <::PortableServer::POA>::deactivate_object_in_params_ [1] = {
	{ "oid", Type <String>::type_code }
};

const Operation ProxyTraits <::PortableServer::POA>::operations_ [] = {
	{ "activate_object", { activate_object_in_params_, countof (activate_object_in_params_) }, {0, 0}, Type <String>::type_code, RqProcWrapper < ::PortableServer::POA, activate_object_request> },
	{ "deactivate_object", { deactivate_object_in_params_, countof (deactivate_object_in_params_) }, {0, 0}, Type <void>::type_code, RqProcWrapper < ::PortableServer::POA, deactivate_object_request> }
};

const Char* const ProxyTraits <::PortableServer::POA>::interfaces_ [] = {
	::PortableServer::POA::repository_id_
};

template <>
const InterfaceMetadata ProxyFactoryImpl <::PortableServer::POA>::metadata_ = {
	{ProxyTraits <::PortableServer::POA>::interfaces_, countof (ProxyTraits <::PortableServer::POA>::interfaces_)},
	{ProxyTraits <::PortableServer::POA>::operations_, countof (ProxyTraits <::PortableServer::POA>::operations_)}
};

}
}

NIRVANA_EXPORT (_exp_PortableServer_POA, PortableServer::POA::repository_id_, CORBA::AbstractBase, ::CORBA::Internal::ProxyFactoryImpl <PortableServer::POA>)
