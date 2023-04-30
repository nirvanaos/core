/// \file Custom marshaling for PortableServer.idl
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
#ifndef NIRVANA_ORB_CORE_PORTABLESERVER_P_NATIVE_H_
#define NIRVANA_ORB_CORE_PORTABLESERVER_P_NATIVE_H_

#include <ORB/ServantProxyObject.h>

namespace CORBA {
namespace Internal {

inline
PortableServer::ServantBase::_ref_type Proxy <PortableServer::POA>::get_servant () const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_get_servant), 3);
	_call->invoke ();
	check_request (_call);
	Object::_ref_type _ret;
	Type <Object>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return Core::object2servant (_ret);
}

void Proxy <PortableServer::POA>::__rq_get_servant (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	_call->unmarshal_end ();
	Object::_ref_type ret;
	{
		Environment _env;
		Type <Object>::C_ret _ret (
			(_servant->_epv ().epv.get_servant) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&_env));
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <Object>::marshal_out (ret, _call);
}

inline
void Proxy <PortableServer::POA>::set_servant (PortableServer::Servant p_servant) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_set_servant), 3);
	Type <Object>::marshal_in (Core::servant2object (p_servant), _call);
	_call->invoke ();
	check_request (_call);
	_call->unmarshal_end ();
}

void Proxy <PortableServer::POA>::__rq_set_servant (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	Object::_ref_type p_servant;
	Type <Object>::unmarshal (_call, p_servant);
	_call->unmarshal_end ();
	{
		Environment _env;
			(_servant->_epv ().epv.set_servant) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <Object>::C_in (p_servant),
				&_env);
		_env.check ();
	}
}

inline
PortableServer::ObjectId Proxy <PortableServer::POA>::activate_object (PortableServer::Servant p_servant) const
{
	Object::_ptr_type proxy = Core::servant2object (p_servant);
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_activate_object), 3);
	Type <Object>::marshal_in (proxy, _call);
	_call->invoke ();
	check_request (_call);
	PortableServer::ObjectId _ret;
	Type <PortableServer::ObjectId>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return _ret;
}

void Proxy <PortableServer::POA>::__rq_activate_object (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	Object::_ref_type p_servant;
	Type <Object>::unmarshal (_call, p_servant);
	_call->unmarshal_end ();
	PortableServer::ObjectId ret;
	{
		Environment _env;
		Type <PortableServer::ObjectId>::C_ret _ret = 
			(_servant->_epv ().epv.activate_object) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <Object>::C_in (p_servant),
				&_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <PortableServer::ObjectId>::marshal_out (ret, _call);
}

inline
void Proxy <PortableServer::POA>::activate_object_with_id (const PortableServer::ObjectId& oid, PortableServer::Servant p_servant) const
{
	Object::_ptr_type proxy = Core::servant2object (p_servant);
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_activate_object_with_id), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	Type <Object>::marshal_in (proxy, _call);
	_call->invoke ();
	check_request (_call);
	_call->unmarshal_end ();
}

void Proxy <PortableServer::POA>::__rq_activate_object_with_id (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	Object::_ref_type p_servant;
	Type <Object>::unmarshal (_call, p_servant);
	_call->unmarshal_end ();
	{
		Environment _env;
		(_servant->_epv ().epv.activate_object_with_id) (
			static_cast <Bridge <PortableServer::POA>*> (&_servant),
			&Type <PortableServer::ObjectId>::C_in (oid),
			&Type <Object>::C_in (p_servant), &_env);
		_env.check ();
	}
}

inline
PortableServer::ObjectId Proxy <PortableServer::POA>::servant_to_id (PortableServer::Servant p_servant) const
{
	Object::_ptr_type proxy = Core::servant2object (p_servant);
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_servant_to_id), 3);
	Type <Object>::marshal_in (proxy, _call);
	_call->invoke ();
	check_request (_call);
	PortableServer::ObjectId _ret;
	Type <PortableServer::ObjectId>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return _ret;
}

void Proxy <PortableServer::POA>::__rq_servant_to_id (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	Object::_ref_type p_servant;
	Type <Object>::unmarshal (_call, p_servant);
	_call->unmarshal_end ();
	PortableServer::ObjectId ret;
	{
		Environment _env;
		Type <PortableServer::ObjectId>::C_ret _ret =
			(_servant->_epv ().epv.servant_to_id) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <Object>::C_in (p_servant),
				&_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <PortableServer::ObjectId>::marshal_out (ret, _call);
}

inline
Object::_ref_type Proxy <PortableServer::POA>::servant_to_reference (PortableServer::Servant p_servant) const
{
	Object::_ptr_type proxy = Core::servant2object (p_servant);
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_servant_to_reference), 3);
	Type <Object>::marshal_in (proxy, _call);
	_call->invoke ();
	check_request (_call);
	Object::_ref_type _ret;
	Type <Object>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return _ret;
}

void Proxy <PortableServer::POA>::__rq_servant_to_reference (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	Object::_ref_type p_servant;
	Type <Object>::unmarshal (_call, p_servant);
	_call->unmarshal_end ();
	Object::_ref_type ret;
	{
		Environment _env;
		Type <Object>::C_ret _ret =
			(_servant->_epv ().epv.servant_to_reference) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <Object>::C_in (p_servant),
				&_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <Object>::marshal_out (ret, _call);
}

inline
PortableServer::ServantBase::_ref_type Proxy <PortableServer::POA>::reference_to_servant (Object::_ptr_type reference) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_reference_to_servant), 3);
	Type <Object>::marshal_in (reference, _call);
	_call->invoke ();
	check_request (_call);
	Object::_ref_type _ret;
	Type <Object>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return Core::object2servant (_ret);
}

void Proxy <PortableServer::POA>::__rq_reference_to_servant (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	Object::_ref_type reference;
	Type <Object>::unmarshal (_call, reference);
	_call->unmarshal_end ();
	Object::_ref_type ret;
	{
		Environment _env;
		Type <Object>::C_ret _ret =
			(_servant->_epv ().epv.reference_to_servant) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <Object>::C_in (reference),
				&_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <Object>::marshal_out (ret, _call);
}

inline
PortableServer::ServantBase::_ref_type Proxy <PortableServer::POA>::id_to_servant (const PortableServer::ObjectId& oid) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_id_to_servant), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	_call->invoke ();
	check_request (_call);
	Object::_ref_type _ret;
	Type <Object>::unmarshal (_call, _ret);
	_call->unmarshal_end ();
	return Core::object2servant (_ret);
}

void Proxy <PortableServer::POA>::__rq_id_to_servant (
	PortableServer::POA::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	_call->unmarshal_end ();
	Object::_ref_type ret;
	{
		Environment _env;
		Type <Object>::C_ret _ret =
			(_servant->_epv ().epv.id_to_servant) (
				static_cast <Bridge <PortableServer::POA>*> (&_servant),
				&Type <PortableServer::ObjectId>::C_in (oid),
				&_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <Object>::marshal_out (ret, _call);
}

inline
PortableServer::ServantBase::_ref_type Proxy <PortableServer::ServantActivator>::incarnate (
	const PortableServer::ObjectId& oid, PortableServer::POA::_ptr_type adapter) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_incarnate), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	Type <PortableServer::POA>::marshal_in (adapter, _call);
	_call->invoke ();
	check_request (_call);

	// Actually, the method returns Obect, not ServantBase, so it must be called via EPV,
	// not via the client call.
	PortableServer::ServantBase::_ref_type _ret;
	Type <Object>::unmarshal (_call, reinterpret_cast <Object::_ref_type&> (_ret));
	_call->unmarshal_end ();

	return _ret;
}

void Proxy <PortableServer::ServantActivator>::__rq_incarnate (
	PortableServer::ServantActivator::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	PortableServer::POA::_ref_type adapter;
	Type <PortableServer::POA>::unmarshal (_call, adapter);
	_call->unmarshal_end ();
	PortableServer::ServantBase::_ref_type _ret = _servant->incarnate (oid, adapter);
	// Marshal output
	Object::_ref_type obj = Core::servant2object (_ret);
	Type <Object>::marshal_out (obj, _call);
}

inline
void Proxy <PortableServer::ServantActivator>::etherealize (
	const PortableServer::ObjectId& oid, PortableServer::POA::_ptr_type adapter,
	PortableServer::Servant serv, Boolean cleanup_in_progress,
	Boolean remaining_activations) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_etherealize), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	Type <PortableServer::POA>::marshal_in (adapter, _call);
	Type <Object>::marshal_in (Core::servant2object (serv), _call);
	Type <Boolean>::marshal_in (cleanup_in_progress, _call);
	Type <Boolean>::marshal_in (remaining_activations, _call);
	_call->invoke ();
	check_request (_call);
	_call->unmarshal_end ();
}

void Proxy <PortableServer::ServantActivator>::__rq_etherealize (
	PortableServer::ServantActivator::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	PortableServer::POA::_ref_type adapter;
	Type <PortableServer::POA>::unmarshal (_call, adapter);
	Object::_ref_type obj;
	Type <Object>::unmarshal (_call, obj);
	Boolean cleanup_in_progress;
	Boolean remaining_activations;
	Type <Boolean>::unmarshal (_call, cleanup_in_progress);
	Type <Boolean>::unmarshal (_call, remaining_activations);
	_call->unmarshal_end ();
	
	// The ServantActivator implementation must be in the same synchronization context as the servant,
	// otherwise an exception will be thrown.
	PortableServer::ServantBase::_ref_type serv = Core::object2servant (obj);

	_servant->etherealize (oid, adapter, serv, cleanup_in_progress, remaining_activations);
}

inline
PortableServer::ServantBase::_ref_type Proxy <PortableServer::ServantLocator>::preinvoke (
	const PortableServer::ObjectId& oid, PortableServer::POA::_ptr_type adapter,
	const Identifier& operation, PortableServer::ServantLocator::Cookie& the_cookie) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_preinvoke), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	Type <PortableServer::POA>::marshal_in (adapter, _call);
	Type <Identifier>::marshal_in (operation, _call);
	_call->invoke ();
	check_request (_call);

	_call->unmarshal (alignof (PortableServer::ServantLocator::Cookie),
		sizeof (PortableServer::ServantLocator::Cookie), &the_cookie);

	// Actually, the method returns Obect, not ServantBase, so it must be called via EPV,
	// not via the client call.
	PortableServer::ServantBase::_ref_type _ret;
	Type <Object>::unmarshal (_call, reinterpret_cast <Object::_ref_type&> (_ret));
	_call->unmarshal_end ();
	return _ret;
}

void Proxy <PortableServer::ServantLocator>::__rq_preinvoke (
	PortableServer::ServantLocator::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	PortableServer::POA::_ref_type adapter;
	Type <PortableServer::POA>::unmarshal (_call, adapter);
	Identifier operation;
	Type <Identifier>::unmarshal (_call, operation);
	_call->unmarshal_end ();
	PortableServer::ServantLocator::Cookie the_cookie = nullptr;

	PortableServer::ServantBase::_ref_type _ret = _servant->preinvoke (oid, adapter, operation, the_cookie);

	_call->marshal (alignof (PortableServer::ServantLocator::Cookie),
		sizeof (PortableServer::ServantLocator::Cookie), &the_cookie);

	Object::_ref_type obj = Core::servant2object (_ret);
	Type <Object>::marshal_out (obj, _call);
}

inline
void Proxy <PortableServer::ServantLocator>::postinvoke (
	const PortableServer::ObjectId& oid, PortableServer::POA::_ptr_type adapter, const Identifier& operation,
	PortableServer::ServantLocator::Cookie the_cookie, PortableServer::Servant the_servant) const
{
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_postinvoke), 3);
	Type <PortableServer::ObjectId>::marshal_in (oid, _call);
	Type <PortableServer::POA>::marshal_in (adapter, _call);
	Type <Identifier>::marshal_in (operation, _call);
	_call->marshal (alignof (PortableServer::ServantLocator::Cookie),
		sizeof (PortableServer::ServantLocator::Cookie), &the_cookie);
	Type <Object>::marshal_in (Core::servant2object (the_servant), _call);
	_call->invoke ();
	check_request (_call);
	_call->unmarshal_end ();
}

void Proxy <PortableServer::ServantLocator>::__rq_postinvoke (
	PortableServer::ServantLocator::_ptr_type _servant, IORequest::_ptr_type _call)
{
	PortableServer::ObjectId oid;
	Type <PortableServer::ObjectId>::unmarshal (_call, oid);
	PortableServer::POA::_ref_type adapter;
	Type <PortableServer::POA>::unmarshal (_call, adapter);
	Identifier operation;
	Type <Identifier>::unmarshal (_call, operation);
	PortableServer::ServantLocator::Cookie the_cookie;
	_call->unmarshal (alignof (PortableServer::ServantLocator::Cookie),
		sizeof (PortableServer::ServantLocator::Cookie), &the_cookie);
	Object::_ref_type obj;
	Type <Object>::unmarshal (_call, obj);
	_call->unmarshal_end ();

	// The ServantActivator implementation must be in the same synchronization context as the servant,
	// otherwise an exception will be thrown.
	PortableServer::ServantBase::_ref_type the_servant = Core::object2servant (obj);
	_servant->postinvoke (oid, adapter, operation, the_cookie, the_servant);
}

}
}

#endif
