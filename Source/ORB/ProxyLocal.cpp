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
#include "ProxyLocal.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

using namespace ::Nirvana::Core;

inline void ProxyLocal::non_existent_request (ProxyLocal* servant, IORequest_ptr _rq,
	::Nirvana::ConstPointer in_params,
	Unmarshal_var unmarshaler,
	::Nirvana::Pointer out_params)
{
	Boolean _ret = servant->servant_->_non_existent ();
	BooleanRet& _out = *(BooleanRet*)out_params;
	Marshal_var _m;
	_marshal_out (_ret, Marshal::_nil (), _out._ret);
}

const Operation ProxyLocal::object_ops_ [3] = {
	{ op_get_interface_, {0, 0}, {0, 0}, TypeI <InterfaceDef>::type_code, nullptr },
	{ op_is_a_, {&is_a_param_, 1}, {0, 0}, Type <Boolean>::type_code, nullptr },
	{ op_non_existent_, {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <ProxyLocal, non_existent_request> }
};

}
}
}
