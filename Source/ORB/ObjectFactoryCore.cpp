#include "ObjectFactoryCore.h"

namespace CORBA {
namespace Nirvana {

Bridge <ObjectFactory>* const g_object_factory = STATIC_BRIDGE (::Nirvana::Core::ObjectFactoryCore, ObjectFactory);

}
}
