/// \file g_runtime_support.cpp
/// The g_runtime_support object implementation.

#include "RuntimeSupportImpl.h"
#include "Thread.h"
#include <Nirvana/RuntimeSupport_s.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

class RuntimeSupport :
	public ::CORBA::Nirvana::ServantStatic <RuntimeSupport, Nirvana::RuntimeSupport>
{
	static RuntimeSupportImpl& get_impl ()
	{
		return Thread::current ().runtime_support ();
	}
public:
	RuntimeProxy_var runtime_proxy_get (const void* obj)
	{
		return get_impl ().runtime_proxy_get (obj);
	}

	void runtime_proxy_remove (const void* obj)
	{
		get_impl ().runtime_proxy_remove (obj);
	}
};

}
}

NIRVANA_EXPORT (_exp_Nirvana_g_runtime_support, "Nirvana/g_runtime_support", Nirvana::RuntimeSupport, Nirvana::Core::RuntimeSupport)

