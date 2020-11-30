#include "ObjectFactory.h"
#include "Thread.inl"
#include <Nirvana/OLF.h>

namespace CORBA {
namespace Nirvana {

namespace Core {

using namespace ::Nirvana;
using namespace ::Nirvana::Core;

StatelessCreationFrame*& ObjectFactory::stateless_creation_frame ()
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	assert (ed);
	return ed->stateless_creation_frame_;
}

size_t ObjectFactory::offset_ptr ()
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	assert (ed);
	StatelessCreationFrame* scs = ed->stateless_creation_frame_;
	if (scs)
		return scs->offset;
	else if (!ed->sync_context ()->sync_domain ())
		throw_BAD_INV_ORDER ();
	return 0;
}

}

extern const ::Nirvana::ImportInterfaceT <ObjectFactory> g_object_factory = {
	::Nirvana::OLF_IMPORT_INTERFACE, "CORBA/Nirvana/g_object_factory",
	ObjectFactory::repository_id_, STATIC_BRIDGE (ObjectFactory, Core::ObjectFactory)
};

}
}
