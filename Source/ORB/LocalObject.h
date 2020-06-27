/// \file LocalObject.h
#ifndef NIRVANA_ORB_CORE_LOCALOBJECT_H_
#define NIRVANA_ORB_CORE_LOCALOBJECT_H_

#include "CoreImpl.h"
#include "ProxyLocal.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Core implementation of LocalObject default operations.
class LocalObject :
	public CoreImpl <LocalObject, CORBA::LocalObject, ProxyLocal>
{
	typedef CoreImpl <LocalObject, CORBA::LocalObject, ProxyLocal> Base;
public:
	LocalObject (CORBA::LocalObject_ptr servant, AbstractBase_ptr abstract_base) :
		Base (servant, abstract_base)
	{}

	// LocalObject default implementation

	Boolean _non_existent () const
	{
		return false;
	}
};

}
}
}

#endif
