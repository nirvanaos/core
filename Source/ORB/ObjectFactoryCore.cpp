#include "ObjectFactoryCore.h"

namespace CORBA {
namespace Nirvana {

const ::Nirvana::ImportInterfaceT <ObjectFactory> g_object_factory = { 
	0, nullptr, nullptr, STATIC_BRIDGE (::Nirvana::Core::ObjectFactoryCore, ObjectFactory)
};

}
}
