#include <CORBA/Server.h>
#include <generated/System_s.h>
#include "Binder.h"
#include "Chrono.h"
#include "Thread.inl"

using namespace std;

namespace Nirvana {
namespace Core {

class System :
	public CORBA::servant_traits <Nirvana::System>::ServantStatic <System>
{
public:
	static RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		RuntimeSupportImpl* impl = get_runtime_support ();
		if (impl)
			return impl->runtime_proxy_get (obj);
		else
			return RuntimeProxy::_nil ();
	}

	static void runtime_proxy_remove (const void* obj)
	{
		RuntimeSupportImpl* impl = get_runtime_support ();
		if (impl)
			impl->runtime_proxy_remove (obj);
	}

	static CORBA::Object::_ref_type bind (const string& name)
	{
		return Binder::bind (name);
	}

	static CORBA::Internal::Interface::_ref_type bind_interface (const string& name, const string& iid)
	{
		return Binder::bind_interface (name, iid);
	}

	static uint16_t epoch ()
	{
		return Chrono::epoch ();
	}

	static Duration system_clock ()
	{
		return Chrono::system_clock ();
	}

	static Duration steady_clock ()
	{
		return Chrono::steady_clock ();
	}

	static Duration system_to_steady (const uint16_t& epoch, const Duration& clock)
	{
		return Chrono::system_to_steady (epoch, clock);
	}

	static Duration steady_to_system (const Duration& steady)
	{
		return Chrono::steady_to_system (steady);
	}

	static Duration deadline ()
	{
		return Thread::current ().exec_domain ()->deadline ();
	}

	static int* error_number ()
	{
		return &Thread::current ().exec_domain ()->runtime_global_.error_number;
	}

	Memory::_ref_type create_heap (uint16_t granularity)
	{
		throw_NO_IMPLEMENT ();
	}

private:
	static RuntimeSupportImpl* get_runtime_support ()
	{
		Thread* thread = Thread::current_ptr ();
		if (thread)
			return &thread->runtime_support ();
		else
			return nullptr;
	}
};

}

extern const ImportInterfaceT <System> g_system = { OLF_IMPORT_INTERFACE, "Nirvana/g_system", System::repository_id_, NIRVANA_STATIC_BRIDGE (System, Core::System) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_system, "Nirvana/g_system", Nirvana::System, Nirvana::Core::System)
