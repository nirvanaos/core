#include "ObjectFactoryCore.h"

namespace CORBA {
namespace Nirvana {

ObjectFactory_ptr g_object_factory = InterfaceStatic < ::Nirvana::Core::ObjectFactoryCore, ObjectFactory>::_bridge ();

}
}
