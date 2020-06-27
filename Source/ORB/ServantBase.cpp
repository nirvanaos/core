#include "ServantBase.h"
#include "POA.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

::PortableServer::POA_var ServantBase::_default_POA () const
{
	return ::PortableServer::POA::_duplicate (g_root_POA);
}

}
}
}
