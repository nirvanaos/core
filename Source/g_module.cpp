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

	void _add_ref ()
	{}

	void _remove_ref ()
	{}
};

}

extern const ImportInterfaceT <Module> g_module = { OLF_IMPORT_INTERFACE, nullptr, nullptr, STATIC_BRIDGE (Module, Core::Module) };

}
