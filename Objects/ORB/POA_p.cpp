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
#include <core.h>
#include <CORBA/Proxy/Proxy.h>
#include <CORBA/POA_s.h>
#include <ORB/ServantBase.h>

NIRVANA_EXPORT (_exp_PortableServer_POA_ServantAlreadyActive_TC, CORBA::Nirvana::RepIdOf <CORBA::Nirvana::Definitions <PortableServer::POA>::ServantAlreadyActive>::repository_id_,
CORBA::TypeCode, CORBA::Nirvana::TypeCodeException <CORBA::Nirvana::Definitions <PortableServer::POA>::ServantAlreadyActive, false>)

NIRVANA_EXPORT (_exp_PortableServer_POA_ObjectNotActive_TC, CORBA::Nirvana::RepIdOf <CORBA::Nirvana::Definitions <PortableServer::POA>::ObjectNotActive>::repository_id_,
CORBA::TypeCode, CORBA::Nirvana::TypeCodeException <CORBA::Nirvana::Definitions <PortableServer::POA>::ObjectNotActive, false>)

namespace CORBA {
namespace Nirvana {

IMPLEMENT_PROXY_FACTORY (::PortableServer::, POA);

template <>
struct ProxyTraits <::PortableServer::POA>
{
	static const Operation operations_ [];
	static const Char* const interfaces_ [];

	// string activate_object (PortableServer::Servant p_servant);

	struct activate_object_in
	{
		Interface* p_servant;
	};

	static const Parameter activate_object_in_params_ [];

	struct activate_object_out
	{
		ABI <String> _ret;
	};

	static void activate_object_request (::PortableServer::POA_ptr _servant,
		IORequest_ptr _call,
		::Nirvana::ConstPointer _in_ptr,
		Unmarshal_var& _u,
		::Nirvana::Pointer _out_ptr)
	{
		const activate_object_in& _in = *(const activate_object_in*)_in_ptr;

		Object_var p_servant;
		Type <Object>::unmarshal (_in.p_servant, _u, p_servant);
		_u = Unmarshal::_nil ();
		String ret;
		{
			Environment _env;
			Type <String>::C_ret _ret = (_servant->_epv ().epv.activate_object) (&_servant, &Object_ptr (p_servant), &_env);
			_env.check ();
			ret = _ret;
		}
		// Marshal output
		activate_object_out& _out = *(activate_object_out*)_out_ptr;
		Marshal_ptr _m = _call->marshaler ();
		Type <String>::marshal_out (ret, _m, _out._ret);
	}

	// void deactivate_object (string oid);

	struct deactivate_object_in
	{
		ABI <String> oid;
	};

	static const Parameter deactivate_object_in_params_ [];

	static void deactivate_object_request (::PortableServer::POA_ptr _servant,
		IORequest_ptr _call,
		::Nirvana::ConstPointer _in_ptr,
		Unmarshal_var& _u,
		::Nirvana::Pointer _out_ptr)
	{
		const deactivate_object_in& _in = *(const deactivate_object_in*)_in_ptr;
		String oid;
		Type <String>::unmarshal (_in.oid, _u, oid);
		_u = Unmarshal::_nil ();
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
	Proxy (IOReference_ptr proxy_manager, CORBA::UShort interface_idx) :
		Base (proxy_manager, interface_idx)
	{}

	Object_ptr servant2object (PortableServer::Servant servant)
	{
		PortableServer::Servant ps = servant->__core_servant ();
		Core::ServantBase* core_obj = static_cast <Core::ServantBase*> (&ps);
		return core_obj->get_proxy ();
	}

	// Returns nil for objects from other domains and for local objects
	PortableServer::ServantBase_var object2servant (Object_ptr obj)
	{
		Environment _env;
		I_ret <PortableServer::ServantBase> _ret = (obj->_epv ().internal.get_servant) (&obj, &_env);
		_env.check ();
		return _ret;
	}

	String activate_object (PortableServer::Servant p_servant)
	{
		Object_ptr proxy = servant2object (p_servant);
		Traits::activate_object_in _in;
		Marshal_var _m = _target ()->create_marshaler ();
		Type <Object>::marshal_in (proxy, _m, _in.p_servant);
		Traits::activate_object_out _out;
		Unmarshal_var _u = _target ()->call (_make_op_idx (0),
			&_in, sizeof (_in), _m, &_out, sizeof (_out));
		String _ret;
		Type <String>::unmarshal (_out._ret, _u, _ret);
		return _ret;
	}

	void deactivate_object (const String& oid)
	{
		Traits::deactivate_object_in _in;
		Marshal_var _m = _target ()->create_marshaler ();
		Type <String>::marshal_in (oid, _m, _in.oid);
		Unmarshal_var _u = _target ()->call (_make_op_idx (1),
			&_in, sizeof (_in), _m, 0, 0);
	}
};

const Parameter ProxyTraits <::PortableServer::POA>::activate_object_in_params_ [1] = {
	{ "p_servant", Type <Object>::type_code }
};

const Parameter ProxyTraits <::PortableServer::POA>::deactivate_object_in_params_ [1] = {
	{ "oid", Type <String>::type_code }
};

const Operation ProxyTraits <::PortableServer::POA>::operations_ [] = {
	{ "activate_object", { activate_object_in_params_, countof (activate_object_in_params_) }, {0, 0}, Type <String>::type_code, RqProcWrapper <::PortableServer::POA, activate_object_request> },
	{ "deactivate_object", { deactivate_object_in_params_, countof (deactivate_object_in_params_) }, {0, 0}, Type <void>::type_code, RqProcWrapper <::PortableServer::POA, deactivate_object_request> }
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

NIRVANA_EXPORT (_exp_PortableServer_POA_ProxyFactory, PortableServer::POA::repository_id_, CORBA::AbstractBase, ::CORBA::Nirvana::ProxyFactoryImpl <PortableServer::POA>)
