/// \file g_runtime_support.cpp
/// The g_runtime_support object implementation.

/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include <RuntimeSupportImpl.h>
#include <Thread.h>
#include <Nirvana/RuntimeSupport_s.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

class RuntimeSupport :
	public ::CORBA::Nirvana::ServantStatic <RuntimeSupport, Nirvana::RuntimeSupport>
{
	static RuntimeSupportImpl* get_impl ()
	{
		Thread* thread = Thread::current_ptr ();
		if (thread)
			return &thread->runtime_support ();
		else
			return nullptr;
	}
public:
	RuntimeProxy_var runtime_proxy_get (const void* obj)
	{
		RuntimeSupportImpl* impl = get_impl ();
		if (impl)
			return impl->runtime_proxy_get (obj);
		else
			return RuntimeProxy::_nil ();
	}

	void runtime_proxy_remove (const void* obj)
	{
		RuntimeSupportImpl* impl = get_impl ();
		if (impl)
			impl->runtime_proxy_remove (obj);
	}
};

}

extern const ImportInterfaceT <RuntimeSupport> g_runtime_support = { OLF_IMPORT_INTERFACE, nullptr, nullptr, 
STATIC_BRIDGE (RuntimeSupport, Core::RuntimeSupport) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_runtime_support, "Nirvana/g_runtime_support", Nirvana::RuntimeSupport, Nirvana::Core::RuntimeSupport)

