#include <core.h>
#include <CORBA/POA.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "tc_impex.h"

namespace CORBA {
namespace Nirvana {

template <>
class TypeCodeException <PortableServer::POA::ServantAlreadyActive> :
	public TypeCodeExceptionImpl <PortableServer::POA::ServantAlreadyActive>
{};

template <>
class TypeCodeException <PortableServer::POA::ObjectNotActive> :
	public TypeCodeExceptionImpl <PortableServer::POA::ObjectNotActive>
{};

namespace Core {

StaticI_ptr <PortableServer::POA> g_root_POA = { 0 };

}

}
}

namespace PortableServer {

typedef CORBA::Nirvana::TypeCodeException <POA::ServantAlreadyActive> TC_POA_ServantAlreadyActive;
typedef CORBA::Nirvana::TypeCodeException <POA::ObjectNotActive> TC_POA_ObjectNotActive;

}

INTERFACE_EXC_IMPEX (PortableServer, POA, ServantAlreadyActive)
INTERFACE_EXC_IMPEX (PortableServer, POA, ObjectNotActive)
