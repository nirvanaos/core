#ifndef NIRVANA_ORB_CORE_PROXYLOCAL_H_
#define NIRVANA_ORB_CORE_PROXYLOCAL_H_

#include "ServantProxyBase.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

class ProxyLocal :
	public ServantProxyBase
{
protected:
	ProxyLocal (LocalObject_ptr servant, AbstractBase_ptr abstract_base) :
		ServantProxyBase (abstract_base, object_ops_, this),
		servant_ (servant)
	{}

private:
	static void non_existent_request (ProxyLocal* servant,
		IORequest_ptr call,
		::Nirvana::ConstPointer in_params,
		Unmarshal_var unmarshaler,
		::Nirvana::Pointer out_params);

private:
	LocalObject_ptr servant_;

	static const Operation object_ops_ [3];
};

}
}
}

#endif
