/// \file ServantBase.h
#ifndef NIRVANA_ORB_CORE_SERVANTBASE_H_
#define NIRVANA_ORB_CORE_SERVANTBASE_H_

#include "CoreImpl.h"
#include "ProxyObject.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Core implementation of ServantBase default operations.
class ServantBase :
	public CoreImpl <ServantBase, PortableServer::ServantBase, ProxyObject>
{
	typedef CoreImpl <ServantBase, PortableServer::ServantBase, ProxyObject> Base;
public:
	using Skeleton <ServantBase, PortableServer::ServantBase>::__get_interface;
	using Skeleton <ServantBase, PortableServer::ServantBase>::__is_a;

	ServantBase (PortableServer::Servant servant) :
		Base (servant)
	{}

	// ServantBase default implementation

	PortableServer::Servant __core_servant ()
	{
		return this;
	}

	PortableServer::POA_var _default_POA () const;

	Boolean _non_existent () const
	{
		return false;
	}
};

}
}
}

#endif
