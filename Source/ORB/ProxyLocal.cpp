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
	{ op_get_interface_, {0, 0}, {0, 0}, _tc_InterfaceDef, nullptr },
	{ op_is_a_, {&is_a_param_, 1}, {0, 0}, _tc_boolean, nullptr },
	{ op_non_existent_, {0, 0}, {0, 0}, _tc_boolean, ObjProcWrapper <ProxyLocal, non_existent_request> }
};

}
}
}
