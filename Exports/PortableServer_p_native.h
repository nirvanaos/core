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

#include <ORB/ServantBase.h>

namespace CORBA {
namespace Internal {

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
				&Object::_ptr_type (p_servant), &_env);
		_env.check ();
		ret = _ret;
	}
	// Marshal output
	Type <PortableServer::ObjectId>::marshal_out (ret, _call);
}

PortableServer::ObjectId Proxy <PortableServer::POA>::activate_object (PortableServer::Servant p_servant) const
{
	Object::_ptr_type proxy = PortableServer::Core::servant2object (p_servant);
	IORequest::_ref_type _call = _target ()->create_request (_make_op_idx (__OPIDX_activate_object));
	Type <Object>::marshal_in (proxy, _call);
	_call->invoke ();
	check_request (_call);
	PortableServer::ObjectId _ret;
	Type <PortableServer::ObjectId>::unmarshal (_call, _ret);
	return _ret;
}

}
}


#endif
