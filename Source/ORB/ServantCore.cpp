#include "ServantCore.h"
//#include "POACore.h"

namespace Nirvana {
namespace Core {

PortableServer::POA_ptr ServantCore::_default_POA () const
{
	assert (false);
	return ::PortableServer::POA_ptr::nil (); // InterfaceStatic <POACore, PortableServer::POA>::_get_ptr ();
}

}
}
