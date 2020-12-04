#include <Nirvana/Module_s.h>

namespace Nirvana {
namespace Core {

class Module :
	public ::CORBA::Nirvana::ServantStatic <Module, ::Nirvana::Module>
{
public:
	static const void* base_address ()
	{
		return nullptr;
	}

	template <class I>
	static CORBA::Nirvana::Interface* __duplicate (CORBA::Nirvana::Interface* itf, CORBA::Nirvana::Interface*)
	{
		return itf;
	}

	template <class I>
	static void __release (CORBA::Nirvana::Interface*)
	{}
};

}

extern const ImportInterfaceT <Module> g_module = { OLF_IMPORT_INTERFACE, "Nirvana/g_module", Module::repository_id_, STATIC_BRIDGE (Module, Core::Module) };

}
