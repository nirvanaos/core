#include "ObjectFactoryCore.h"

namespace CORBA {
namespace Nirvana {

ObjectFactory_ptr g_object_factory = STATIC_BRIDGE (::Nirvana::Core::ObjectFactoryCore, ObjectFactory);

}
}
