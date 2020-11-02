#include "ObjectFactory.h"
#include <Nirvana/OLF.h>

namespace CORBA {
namespace Nirvana {

namespace Core {
StatelessCreationFrame* ObjectFactory::stateless_ = 0;
}

extern const ::Nirvana::ImportInterfaceT <ObjectFactory> g_object_factory = {
	::Nirvana::OLF_IMPORT_INTERFACE, "CORBA/Nirvana/g_object_factory",
	ObjectFactory::repository_id_, STATIC_BRIDGE (ObjectFactory, Core::ObjectFactory)
};

}
}
